---
sidebar_position: 2
---

# Installing Mods

Written by **[TheGameAce](https://next.nexusmods.com/profile/TheGameAceReal/mods?gameId=6063)**

In this guide, we'll cover how to install PalSchema mods. This will apply **universally** to PalSchema mods, though it's recommended to double check if an author has any additional installation instructions.

## Checklist
Before installing any PalSchema mods, **make sure you've properly done the following:**

 - **[Install the Experimental Fork of UE4SS](./installation.mdx#ue4ss---correct-version--installation)**
 - **[Install PalSchema](./installation.mdx#installing-palschema--using-palschema-mods)**
 - **Remove Files From Older UE4SS Install**:
    - **Mods Folder Under Win64 or WinGDK Folders** - If you still have a **Mods** folder under **Win64 (Steam)** or **WinGDK (Gamepass)**, it **WILL** cause crashes and prevent proper functionality.
    - **UE4SS.dll, UE4SS-settings.ini Under Win64 or WinGDK** - If you have a **UE4SS-settings.ini** or **UE4SS.dll** in your **Win64 (Steam)** or **WinGDK (Game Pass)**, delete these as they are most likely from an old UE4SS install and **WILL** interfere with your new install, causing the old UE4SS to load instead, ***which in turn causes Pal Schema and its mods to not function correctly.***
 
 Here's a quick screenshot of a **Mods** folder **where it shouldn't be if you've installed everything properly**:
 ![MODS FOLDER IN THE WRONG LOCATION](https://i.gyazo.com/86d4b188ca5e43387252263b3d71a76f.png)
 >Wrong Mods Folder Location
 
 The **ONLY** Mods folder in these directories for both **Win64** and **WinGDK** should be under the **UE4SS** folder, as shown below:
 ![CORRECT MODS FOLDER LOCATION](https://i.gyazo.com/a8cc1428ba5cee8e51069d6d74c0f1f5.png)
 >Correct Mods Folder Location
 
 ## Installing the Mod
 If everything else is done correctly, all you need now is to install your mod(s)! Navigate to **[NexusMods](https://www.nexusmods.com/games/palworld/mods?tag=PalSchema)** or wherever else you might get your PalSchema mods from, and download the mod!

Once the mod is downloaded, you should have a zip folder, likely with the mod's name on it.
![PS Mod Zip Example](https://i.gyazo.com/e7629ecca06ed012af16e8c0f0f64325.png)

While you can click on the zip and then right click and extract, I find that to be a bit messy. My recommended method is to double click the zip file to open it up, which should get you a view like this:

![Zip Inside](https://i.gyazo.com/9bfa40cfa72c138f9ece1bb85b36436c.png)

Bring up your File Explorer in an additional window now (you can right click the folder icon for file explorer on your computer's hotbar and select another folder for this), and navigate back to your Palworld installation and the **Win64** folder if you're on Steam or **WinGDK** folder if you're on Gamepass. From here, navigate into the UE4SS folder, and the **Mods** folder therein.

![Mods Folder Inside](https://i.gyazo.com/e593fa1348527ca329505b5c4e5f0487.png)
>Inside of the Mods Folder; proper installation path from the Palworld folder can be seen therein.

Above you can see an example from my **Mods** folder, as well as the appropriate path to get here. PalSchema is highlighted, and if you've properly installed it, it should look like this.

Now, open up the PalSchema folder. Amongst the few folders in there, you want to go into the folder labeled **mods**. Pull up the other File Explorer window (which should still be inside of the zip file from the mod you downloaded), and drag and drop the folder contained into interior of the **mods** folder. The following should be the result (minus the extra mod folders I have):
![Mods folder interior; successful installation](https://i.gyazo.com/528c762409c3c5f56e05a75b9c4fb659.png)
>Correct Installation of a PalSchema mod.

## IMPORTANT NOTE
***Some PalSchema mods will be hybrids, utilizing both a mod folder as seen above, and additional assets packaged like a traditional mod. Please make sure you check all files and any installation instructions that come with any mod.***

**Below is an example of a PalSchema-based mod that has additional files necessary for proper functionality. Without all files prescribed by any mod author, you risk crashes or reduced functionality.**
![Nexus Example](https://i.gyazo.com/7225a86cb0e34e79a6c9b87335b86f46.png)
>Remember to check instructions for every mod you download and **READ** over them thoroughly! Incorrect installation **WILL** cause issues!