from __future__ import annotations

import json
from pathlib import Path
from typing import Dict, List, Optional

from .models import Expression, ExpressionMetadata


class ExpressionRepository:
    """Persists expressions and their metadata to disk."""

    def __init__(self, db_path: Path) -> None:
        self.db_path = db_path
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        if not self.db_path.exists():
            self._write_raw({})

    def _write_raw(self, payload: Dict[str, Dict]) -> None:
        self.db_path.write_text(json.dumps(payload, indent=2, sort_keys=True))

    def _read_raw(self) -> Dict[str, Dict]:
        if not self.db_path.exists():
            return {}
        return json.loads(self.db_path.read_text() or "{}")

    def list(self) -> List[Expression]:
        data = self._read_raw()
        return [self._deserialize(expr_id, payload) for expr_id, payload in data.items()]

    def get(self, expr_id: str) -> Optional[Expression]:
        data = self._read_raw()
        payload = data.get(expr_id)
        if not payload:
            return None
        return self._deserialize(expr_id, payload)

    def upsert(self, expression: Expression) -> None:
        data = self._read_raw()
        data[expression.id] = self._serialize(expression)
        self._write_raw(data)

    def delete(self, expr_id: str) -> None:
        data = self._read_raw()
        if expr_id in data:
            del data[expr_id]
            self._write_raw(data)

    def _serialize(self, expression: Expression) -> Dict:
        return {
            "original_filename": expression.original_filename,
            "processed_filename": expression.processed_filename,
            "original_path": str(expression.original_path),
            "processed_path": str(expression.processed_path),
            "metadata": expression.metadata.model_dump(),
        }

    def _deserialize(self, expr_id: str, payload: Dict) -> Expression:
        metadata = ExpressionMetadata(**payload.get("metadata", {}))
        return Expression(
            id=expr_id,
            original_filename=payload["original_filename"],
            processed_filename=payload["processed_filename"],
            original_path=Path(payload["original_path"]),
            processed_path=Path(payload["processed_path"]),
            metadata=metadata,
        )
