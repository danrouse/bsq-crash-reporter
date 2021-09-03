document.addEventListener('DOMContentLoaded', () => {
  document.getElementById('search-input').addEventListener('submit', (evt) => {
    evt.preventDefault();
    var elems = evt.target.elements;
    if (elems.name.value) {
      location.href = `/${elems.type.value}/${elems.name.value}`;
    }
  });
  const _timeago = timeago();
  Array.from(document.getElementsByTagName('time')).forEach((elem) => {
    const date = new Date(elem.dateTime);
    elem.innerText = `${date.toLocaleDateString()} ${date.toLocaleTimeString()} (${_timeago.format(date)})`;
  });
});
