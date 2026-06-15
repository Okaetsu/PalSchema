---
sidebar_position: 10
---

# TSoftObjectPtr

TSoftObjectPtr Properties in Pal Schema are defined as strings that point to an asset path.

## Example

`Pal/Content/Pal/DataTable/Character/DT_PalCharacterIconDataTable.uasset`

`Icon` in `DT_PalCharacterIconDataTable` is a TSoftObjectPtr Property. You can tell from the long path that looks something like this: `/Game/Pal/Texture/PalIcon/Normal/T_Anubis_icon_normal.T_Anubis_icon_normal`, in FModel this would translate to `Pal/Content/Pal/Texture/PalIcon/Normal/T_Anubis_icon_normal.T_Anubis_icon_normal`.

In Pal Schema you want to convert the beginning `Pal/Content` into `/Game/`. Make sure it has a forward slash in the beginning, otherwise your path will not be read correctly, so don't do something like this: `Game/etc`

```json
{
    "DT_PalCharacterIconDataTable": {
        "Anubis": {
            "Icon": "/Game/Pal/Texture/PalIcon/Normal/T_PinkCat_icon_normal.T_PinkCat_icon_normal"
        }
    }
}
```

As you can see, we just use a string right after Icon, rather than the complex object that can be seen in FModel.