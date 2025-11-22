const form = document.getElementById('convertForm');
const output = document.getElementById('output');

form.addEventListener('submit', async (e) => {
  e.preventDefault();
  output.innerHTML = 'Converting... ⏳';

  const file = document.getElementById('fileInput').files[0];
  const format = document.getElementById('format').value;
  const quality = document.getElementById('quality').value;

  if (!file) {
    output.textContent = 'Please select a file.';
    return;
  }

  try {
    const resp = await fetch(`/convert/${format}?quality=${quality}`, {
      method: 'POST',
      headers: { 'Content-Type': file.type },
      body: file,
    });

    if (!resp.ok) throw new Error('Conversion failed');

    const blob = await resp.blob();
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = `converted.${format}`;
    link.textContent = `Download converted.${format}`;
    output.innerHTML = '';
    output.appendChild(link);
  } catch (err) {
    output.textContent = `Error: ${err.message}`;
  }
});