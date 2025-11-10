from __future__ import annotations

from pathlib import Path
from typing import List, Optional

from pydantic import BaseModel, Field


class ExpressionMetadata(BaseModel):
    """Describes editable metadata for a GIF expression."""

    title: str = Field(default="", description="Short name for the GIF expression")
    description: str = Field(default="", description="Longer notes about the GIF expression")
    tags: List[str] = Field(default_factory=list, description="Free-form tags for search and grouping")


class Expression(BaseModel):
    """Represents a GIF expression stored by the application."""

    id: str
    original_filename: str
    processed_filename: str
    original_path: Path
    processed_path: Path
    metadata: ExpressionMetadata


class ExpressionUpdate(BaseModel):
    """Payload for updating expression metadata."""

    title: Optional[str] = None
    description: Optional[str] = None
    tags: Optional[List[str]] = None
