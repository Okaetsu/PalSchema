---
sidebar_position: 1
---

# Features

PalSchema is a mod that allows runtime modification of assets in Palworld with json files without introducing conflicts with other mods that modify the same game files. As the name suggests, heavy use of json schema is utilized, so mod creators can have autocompletion and error checking to make creation of table mods easier.

---

- **Runtime Modification of Assets** - Main purpose of PalSchema is to be able to modify the game's assets via `.json` or `.jsonc` files. This includes `DT (Data Table)`, and `BP (Blueprint)` assets. Note that when it comes to Data Tables, you'll also be able to add new rows.

- **Stability throughout Updates** - As we know, Palworld is in Early Access and this means frequent updates which results in traditional `.pak` mods breaking often. Lua mods are a great stable alternative, but they require coding knowledge and have some limitations currently on what they can and can't do. PalSchema aims to bring stability to the table while also making it super easy to create and modify existing mods.

- **Custom Localization** - Ever wondered when a game plans on adding support for your language? If it doesn't exist, you'll be able to create it with PalSchema very easily.

- **Auto-reload** - When enabled via the config, PalSchema detects any new changes made to your json files, reloads and applies those changes all while the game is still running which means you'll never have to exit the game to test for new changes, however there are some exceptions to this.

- **Pak Reader Redirection** - Allows you to package any PalSchema related `.pak` files with your mod by creating a `paks` folder within your PalSchema mod. PalSchema will then read any `.pak` files you have within that `paks` folder, rather than having to do complicated setups via the default `Paks` location.