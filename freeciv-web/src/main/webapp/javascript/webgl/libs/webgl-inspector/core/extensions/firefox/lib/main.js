const { data, uri } = require('sdk/self');
const { PageMod } = require('sdk/page-mod');
const tabs = require('sdk/tabs');

function attach (tab) {
  let mod = PageMod({
    include: tab.url,
    contentScriptFile: [data.url('load.js')],
    contentScriptWhen: 'start',
    contentScriptOptions: {
      myJS: [data.url('gli.all.js'), data.url('shim.js')],
      myCSS: data.url('gli.all.css'),
    },
  });
  tab.once('load', mod.destroy);
  tab.reload();
};

require('menuitems').Menuitem({
  id: 'webgl-inspector',
  label: 'WebGL Inspector',
  onCommand: function () {
    let tab = tabs.activeTab;
    tab.url ? attach(tab) : tab.once('ready', attach.bind(null, tab));
  },
  menuid: 'menuWebDeveloperPopup',
  insertbefore: 'devToolsEndSeparator',
});

