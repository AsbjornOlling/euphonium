import { Link } from "preact-router/match";
import { useState, useEffect } from "preact/hooks";
import { getPlugins } from "../../api/euphonium/api";
import { PluginEntry } from "../../api/euphonium/models";
import css from "./SideBar.module.scss";

const SideBarItem = ({ displayName = "", name = "", type = "" }) => {
  return (
    <Link
      activeClassName={css.sideBar__itemselected}
      className={css.sideBar__itemwrapper}
      href={`/web/plugin/${name}`}
    >
      <div className={css.sideBar__item}>{displayName}</div>
    </Link>
  );
};

export default () => {
  const [systemPlugins, setSystemPlugins] = useState<PluginEntry[]>([]);
  const [plugins, setPlugins] = useState<PluginEntry[]>([]);

  useEffect(() => {
    getPlugins().then((data) => {
      setSystemPlugins(data.filter((e) => e.type == "system"));
      setPlugins(data.filter((e) => e.type == "plugin"));
    });
  }, []);

  return (
    <div className={css.sideBar}>
      <div className={css.sideBar__header}>Euphonium 🎺</div>
      <div className={css.sideBar__subheader}>available apps</div>
      <SideBarItem name="webradio2" displayName="Web Radio" />
      <div className={css.sideBar__subheader}>system modules</div>
      {systemPlugins.map((result) => (
        <SideBarItem {...result} />
      ))}

      <div className={css.sideBar__subheader}>installed plugins</div>
      {plugins.map((result) => (
        <SideBarItem {...result} />
      ))}
    </div>
  );
};
