from __future__ import annotations

import shutil
import uuid
from pathlib import Path
from typing import List

from fastapi import Depends, FastAPI, File, Form, HTTPException, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse

from .conversion import GifConverter
from .models import Expression, ExpressionMetadata, ExpressionUpdate
from .storage import ExpressionRepository

DATA_DIR = Path("data")
UPLOADS_DIR = DATA_DIR / "uploads"
PROCESSED_DIR = DATA_DIR / "processed"
DB_PATH = DATA_DIR / "expressions.json"
CONVERSION_SCRIPT = Path("scripts/convert_gif.py")


app = FastAPI(title="RobotEye GIF Expressions")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


def get_repository() -> ExpressionRepository:
    return ExpressionRepository(DB_PATH)


def get_converter() -> GifConverter:
    return GifConverter(CONVERSION_SCRIPT)


@app.get("/api/expressions", response_model=List[Expression])
def list_expressions(repo: ExpressionRepository = Depends(get_repository)) -> List[Expression]:
    return repo.list()


@app.post("/api/expressions", response_model=Expression, status_code=201)
async def create_expression(
    gif: UploadFile = File(...),
    title: str = Form(""),
    description: str = Form(""),
    tags: str = Form(""),
    repo: ExpressionRepository = Depends(get_repository),
    converter: GifConverter = Depends(get_converter),
) -> Expression:
    expression_id = str(uuid.uuid4())
    UPLOADS_DIR.mkdir(parents=True, exist_ok=True)
    PROCESSED_DIR.mkdir(parents=True, exist_ok=True)

    original_filename = f"{expression_id}_{gif.filename or 'upload.gif'}"
    original_path = UPLOADS_DIR / original_filename

    with original_path.open("wb") as buffer:
        shutil.copyfileobj(gif.file, buffer)

    processed_filename = f"{expression_id}_processed.gif"
    processed_path = PROCESSED_DIR / processed_filename

    converter.convert(original_path, processed_path)

    metadata = ExpressionMetadata(
        title=title.strip(),
        description=description.strip(),
        tags=_split_tags(tags),
    )

    expression = Expression(
        id=expression_id,
        original_filename=original_filename,
        processed_filename=processed_filename,
        original_path=original_path,
        processed_path=processed_path,
        metadata=metadata,
    )
    repo.upsert(expression)
    return expression


@app.put("/api/expressions/{expression_id}", response_model=Expression)
def update_expression(
    expression_id: str,
    payload: ExpressionUpdate,
    repo: ExpressionRepository = Depends(get_repository),
) -> Expression:
    expression = repo.get(expression_id)
    if not expression:
        raise HTTPException(status_code=404, detail="Expression not found")

    if payload.title is not None:
        expression.metadata.title = payload.title.strip()
    if payload.description is not None:
        expression.metadata.description = payload.description.strip()
    if payload.tags is not None:
        expression.metadata.tags = payload.tags

    repo.upsert(expression)
    return expression


@app.delete("/api/expressions/{expression_id}", status_code=204)
def delete_expression(
    expression_id: str,
    repo: ExpressionRepository = Depends(get_repository),
) -> None:
    expression = repo.get(expression_id)
    if not expression:
        raise HTTPException(status_code=404, detail="Expression not found")

    if expression.original_path.exists():
        expression.original_path.unlink()
    if expression.processed_path.exists():
        expression.processed_path.unlink()

    repo.delete(expression_id)


@app.get("/api/expressions/{expression_id}/download")
def download_expression(
    expression_id: str,
    repo: ExpressionRepository = Depends(get_repository),
) -> FileResponse:
    expression = repo.get(expression_id)
    if not expression:
        raise HTTPException(status_code=404, detail="Expression not found")

    if not expression.processed_path.exists():
        raise HTTPException(status_code=500, detail="Processed GIF is missing")

    return FileResponse(
        path=expression.processed_path,
        filename=expression.processed_filename,
        media_type="image/gif",
    )


def _split_tags(tags: str) -> List[str]:
    if not tags:
        return []
    parts = [part.strip() for part in tags.split(",")]
    return [part for part in parts if part]
