# SoftClass

SoftClass properties in PalSchema are defined as strings that point to a soft asset class reference which holds a pointer to the asset class in memory if it's present.

## Example

`Pal/Content/Pal/DataTable/Character/DT_PalBPClass.uasset`

`BPClass` in `DT_PalBPClass` is a SoftClass property. 

Below is an example of what a SoftClass property looks like in FModel:
```json
"BPClass": {
    "AssetPathName": "/Game/Pal/Blueprint/Character/Monster/PalActorBP/Anubis/BP_Anubis.BP_Anubis_C",
    "SubPathString": ""
}
```

You can see that it's a json object which has the fields `AssetPathName` and `SubPathString`. It looks similar to [SoftObject](./softobjectptr.md) property, but if you look at the end of `AssetPathName`, you can see a `_C` which indicates that it's referring to a class.

Remember to always end your path in `_C` when dealing with any Class properties in PalSchema.

PalSchema Example:
```json
{
    "DT_PalBPClass": {
        "Anubis": {
            "BPClass": "/Game/Pal/Blueprint/Character/Monster/PalActorBP/Anubis/BP_Anubis.BP_Anubis_C"
        }
    }
}
```

As you can see, we just use a string right after BPClass, rather than the complex object that can be seen in FModel.