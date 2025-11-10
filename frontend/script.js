const API_BASE = window.API_BASE_URL || `${window.location.protocol}//${window.location.hostname}:8000`;
const API_ROOT = `${API_BASE}/api`;

const uploadForm = document.querySelector('#upload-form');
const gifInput = document.querySelector('#gif-input');
const titleInput = document.querySelector('#title-input');
const descriptionInput = document.querySelector('#description-input');
const tagsInput = document.querySelector('#tags-input');
const gifPreview = document.querySelector('#gif-preview');
const gifPreviewImage = document.querySelector('#gif-preview-image');
const uploadStatus = document.querySelector('#upload-status');
const expressionsList = document.querySelector('#expressions-list');
const expressionsEmpty = document.querySelector('#expressions-empty');
const expressionTemplate = document.querySelector('#expression-template');
const defaultEmptyMarkup = expressionsEmpty?.innerHTML;

let currentPreviewUrl;

function showPreview(file) {
  if (currentPreviewUrl) {
    URL.revokeObjectURL(currentPreviewUrl);
  }
  currentPreviewUrl = URL.createObjectURL(file);
  gifPreviewImage.src = currentPreviewUrl;
  gifPreview.hidden = false;
}

gifInput.addEventListener('change', () => {
  if (gifInput.files && gifInput.files[0]) {
    showPreview(gifInput.files[0]);
  }
});

uploadForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  if (!gifInput.files || !gifInput.files[0]) {
    uploadStatus.textContent = 'Please choose a GIF to upload.';
    return;
  }

  const formData = new FormData();
  formData.append('gif', gifInput.files[0]);
  formData.append('title', titleInput.value);
  formData.append('description', descriptionInput.value);
  formData.append('tags', tagsInput.value);

  uploadStatus.textContent = 'Uploading and processing...';

  try {
    const response = await fetch(`${API_ROOT}/expressions`, {
      method: 'POST',
      body: formData,
    });

    if (!response.ok) {
      throw new Error(`Upload failed (${response.status})`);
    }

    await response.json();
    uploadStatus.textContent = 'GIF uploaded successfully!';
    uploadForm.reset();
    gifPreview.hidden = true;
    await loadExpressions();
  } catch (error) {
    console.error(error);
    uploadStatus.textContent = 'Upload failed. Please try again.';
  }
});

function splitTags(raw) {
  if (!raw.trim()) return [];
  return raw
    .split(',')
    .map((tag) => tag.trim())
    .filter(Boolean);
}

function joinTags(tags) {
  return (tags || []).join(', ');
}

function renderExpression(expression) {
  const element = expressionTemplate.content.firstElementChild.cloneNode(true);
  const image = element.querySelector('.expression-image');
  const titleField = element.querySelector('.metadata-title');
  const descriptionField = element.querySelector('.metadata-description');
  const tagsField = element.querySelector('.metadata-tags');
  const saveButton = element.querySelector('.js-save');
  const deleteButton = element.querySelector('.js-delete');
  const downloadLink = element.querySelector('.js-download');
  const statusField = element.querySelector('.expression-status');

  image.src = `${API_ROOT}/expressions/${expression.id}/download`;
  image.alt = expression.metadata.title ? `${expression.metadata.title} preview` : 'Processed GIF preview';
  titleField.value = expression.metadata.title || '';
  descriptionField.value = expression.metadata.description || '';
  tagsField.value = joinTags(expression.metadata.tags);
  downloadLink.href = `${API_ROOT}/expressions/${expression.id}/download`;
  downloadLink.download = expression.processed_filename;

  saveButton.addEventListener('click', async () => {
    statusField.textContent = 'Saving metadata...';
    saveButton.disabled = true;
    try {
      const payload = {
        title: titleField.value,
        description: descriptionField.value,
        tags: splitTags(tagsField.value),
      };

      const response = await fetch(`${API_ROOT}/expressions/${expression.id}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
      });

      if (!response.ok) {
        throw new Error('Failed to save metadata');
      }

      statusField.textContent = 'Saved!';
    } catch (error) {
      console.error(error);
      statusField.textContent = 'Error saving metadata.';
    } finally {
      saveButton.disabled = false;
      setTimeout(() => (statusField.textContent = ''), 2500);
    }
  });

  deleteButton.addEventListener('click', async () => {
    if (!confirm('Delete this expression?')) return;
    statusField.textContent = 'Deleting...';
    deleteButton.disabled = true;
    try {
      const response = await fetch(`${API_ROOT}/expressions/${expression.id}`, {
        method: 'DELETE',
      });
      if (!response.ok) {
        throw new Error('Failed to delete');
      }
      element.remove();
      await loadExpressions();
    } catch (error) {
      console.error(error);
      statusField.textContent = 'Error deleting expression.';
    } finally {
      deleteButton.disabled = false;
    }
  });

  return element;
}

async function loadExpressions() {
  try {
    const response = await fetch(`${API_ROOT}/expressions`);
    if (!response.ok) {
      throw new Error('Failed to load expressions');
    }
    const expressions = await response.json();
    expressionsList.innerHTML = '';
    if (!expressions.length) {
      expressionsEmpty.hidden = false;
      return;
    }
    expressionsEmpty.hidden = true;
    expressionsEmpty.innerHTML = defaultEmptyMarkup;
    expressions
      .sort((a, b) => {
        const titleA = (a.metadata?.title || '').toLowerCase();
        const titleB = (b.metadata?.title || '').toLowerCase();
        return titleA.localeCompare(titleB);
      })
      .forEach((expression) => {
        expressionsList.appendChild(renderExpression(expression));
      });
  } catch (error) {
    console.error(error);
    expressionsEmpty.hidden = false;
    expressionsEmpty.textContent = 'Failed to load expressions. Ensure the backend is running.';
  }
}

loadExpressions();
