# RobotEye GIF Manager

A minimal full-stack prototype for managing GIF "expressions". The FastAPI backend accepts
GIF uploads, invokes a conversion script, and stores metadata in a JSON database. A vanilla
HTML/JavaScript frontend allows uploading, previewing, editing metadata, deleting entries,
and downloading processed GIFs.

## Project structure

```
backend/              # FastAPI application
  main.py             # API endpoints
  conversion.py       # Conversion script wrapper
  storage.py          # JSON persistence helpers
  models.py           # Pydantic data models
  requirements.txt    # Python dependencies
frontend/             # Static frontend assets
  index.html
  script.js
  styles.css
scripts/              # External utilities
  convert_gif.py      # Toy conversion script that copies the GIF
data/                # Runtime data (uploads, processed GIFs, JSON DB)
```

## Getting started

### Backend

Create a virtual environment and install dependencies:

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r backend/requirements.txt
```

Run the FastAPI server:

```bash
uvicorn backend.main:app --reload
```

The API will be available at `http://localhost:8000`.

### Frontend

The frontend is static and can be opened directly from the filesystem or served by any
static web server. When opened locally, it expects the API to be running on
`http://localhost:8000`. To override the backend URL, define `window.API_BASE_URL` before
loading `script.js`.

For example, using Python's built-in HTTP server:

```bash
cd frontend
python -m http.server 8080
```

Then open `http://localhost:8080` in your browser.

## API overview

- `GET /api/expressions` — list stored expressions
- `POST /api/expressions` — upload a GIF and metadata (multipart form)
- `PUT /api/expressions/{id}` — update metadata
- `DELETE /api/expressions/{id}` — remove expression and associated files
- `GET /api/expressions/{id}/download` — download processed GIF

All expressions are stored in `data/expressions.json` with references to original and
processed GIF paths.
