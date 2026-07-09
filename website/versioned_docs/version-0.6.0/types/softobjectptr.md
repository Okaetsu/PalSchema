# SoftObject

SoftObject properties in PalSchema are defined as strings that point to a soft asset reference which holds a pointer to the asset in memory if it's present.

## Example

`Pal/Content/Pal/DataTable/Character/DT_PalCharacterIconDataTable.uasset`

`Icon` in `DT_PalCharacterIconDataTable` is a SoftObject property.

Below is an example of what a SoftObject property looks like in FModel:
```json
"Icon": {
    "AssetPathName": "/Game/Pal/Texture/PalIcon/Normal/T_Anubis_icon_normal.T_Anubis_icon_normal",
    "SubPathString": ""
}
```

You can see that it's a json object which has the fields `AssetPathName` and `SubPathString` which is different from the [Object](./objectproperty.md) property.

PalSchema Example:
```json
{
    "DT_PalCharacterIconDataTable": {
        "Anubis": {
            "Icon": "/Game/Pal/Texture/PalIcon/Normal/T_Anubis_icon_normal.T_Anubis_icon_normal"
        }
    }
}
```

As you can see, we just use a string right after Icon, rather than the complex object that can be seen in FModel.