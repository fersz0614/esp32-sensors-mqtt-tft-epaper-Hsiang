// Replace HEIC-only cards with browser-friendly JPG previews while keeping the original HEIC link.
document.querySelectorAll('.gallery a[href$=".heic"]').forEach(link => {
  const figure = link.closest('figure');
  if (!figure) return;
  const original = link.getAttribute('href');
  const preview = original.replace(/\.heic$/i, '.jpg');
  const caption = figure.querySelector('figcaption')?.textContent || 'HEIC preview';
  figure.innerHTML = `<a href="${original}" target="_blank" rel="noreferrer"><img src="${preview}" alt="${caption}" loading="lazy"></a><figcaption>${caption} · 點擊開啟原始 HEIC</figcaption>`;
});
