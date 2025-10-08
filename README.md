# PalSchema
PalSchema is a mod that allows modifying of data tables and blueprints in Palworld with json files without introducing conflicts with other mods that modify the same game files. As the name suggests, I've also made heavy use of json schema, so mod creators can have autocompletion and error checking to make creation of table mods easier.

# Installation

In-depth installation guide can be found [here](https://okaetsu.github.io/PalSchema/docs/installation)

# Documentation

Documentation for modders can be found [here](https://okaetsu.github.io/PalSchema/docs/gettingstarted)

# Building from Source

1. You must complete the **Build requirements** over at [UE4SS Docs](https://docs.ue4ss.com/#build-requirements) and make sure your GitHub account is linked to Epic Games for Unreal Engine source access.

2. Create your own fork of PalSchema and clone it.

3. Execute this command: `git submodule update --init --recursive`

4. And then execute the following:
```
xmake f -m Game__Shipping__Win64 -y
xmake project -k vsxmake2022 -m Game__Shipping__Win64 -y
```

Alternatively you can just run `generate_build_files.bat` in build_scripts instead of running the command in Step 4.

# Mods using PalSchema

I'll only include one mod per author to avoid cluttering the list too much.

[True Recipes](https://www.nexusmods.com/palworld/mods/2159) by The Sin of Pride

[Bounty Shop Plus](https://www.nexusmods.com/palworld/mods/2162) by TheGameAce

[Dog Coin Replacer](https://www.nexusmods.com/palworld/mods/2197) by DeltaJordan

[Additional Skins for PalSchema](https://www.nexusmods.com/palworld/mods/2199) by William112792

[Faster Yakumo for PalSchema](https://www.nexusmods.com/palworld/mods/2207) by MelwenMods

[Pal Fusion Mod](https://www.nexusmods.com/palworld/mods/2384) by Primarinabee

[Male Probability 50 for All Pals](https://www.nexusmods.com/palworld/mods/2285) by ZeroRin00

[All legendary head pieces match hexolite head gear](https://www.nexusmods.com/palworld/mods/2295) by Helionhell

[Expensive Palboxes](https://www.nexusmods.com/palworld/mods/708) by marksmango
