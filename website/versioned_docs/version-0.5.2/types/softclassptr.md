---
sidebar_position: 11
---

# TSoftClassPtr

TSoftClassPtr Properties in Pal Schema are defined as strings that point to an asset path. They're basically the same as [TSoftObjectPtr](./softobjectptr.md), except they must end with a `_C`.

## Example

`Pal/Content/Pal/DataTable/Character/DT_PalBPClass.uasset`

`BPClass` in `DT_PalBPClass` is a TSoftClassPtr property. You can tell from the long path that looks something like this: `/Game/Pal/Blueprint/Character/Monster/PalActorBP/Anubis/BP_Anubis.BP_Anubis_C`. Notice how the path ends in a `_C`? This basically tells Unreal that we want a soft pointer to a Class rather than an Object.

In FModel the above path would translate to `Pal/Content/Pal/Blueprint/Character/Monster/PalActorBP/Anubis/BP_Anubis.BP_Anubis_C`.

In Pal Schema you want to convert the beginning `Pal/Content` into `/Game/`. Make sure it has a forward slash in the beginning, otherwise your path will not be read correctly, so don't do something like this: `Game/etc`. Remember to always end your path in `_C` when dealing with TSoftClassPtr properties.

```json
{
    "DT_PalBPClass": {
        "Anubis": {
            "BPClass": "/Game/Pal/Blueprint/Character/Monster/PalActorBP/PinkCat/BP_PinkCat.BP_PinkCat_C"
        }
    }
}
```

As you can see, we just use a string right after BPClass, rather than the complex object that can be seen in FModel.