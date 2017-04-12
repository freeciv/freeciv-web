self.options.myJS.forEach(function (url, i) {
  var script = document.createElement('script');
  script.src = url;
  if (i === 0) script.dataset.gliCssUrl = self.options.myCSS;
  document.head.appendChild(script);
});

