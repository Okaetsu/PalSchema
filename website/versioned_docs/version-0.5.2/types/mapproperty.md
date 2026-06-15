---
sidebar_position: 8
---

# TMap

TMap Properties in Pal Schema are defined as arrays of objects with key value pairs.

## Example

`Pal/Content/Pal/Blueprint/System/BP_PalGameSetting.uasset`

`CharacterRankUpRequiredNumMap` in `BP_PalGameSetting` is a TMap Property. Let's say we want to modify the amount of Pals required for Star Level 1 from 4 to 1.

```json
{
    "BP_PalGameSetting_C": {
        "CharacterRankUpRequiredNumMap": [
            {
                "Key": 1,
                "Value": 1
            }
        ]
    }
}
```

Only the entry with the Key "1" will be modified and the rest of the entries will remain unchanged, if a Key doesn't exist, it will add a new entry to the TMap instead.

It's pretty much the same format as FModel, but there's one difference. You may notice that the "Key" in FModel looks like a string, but in reality you should define it based on the actual type used which might be a bit difficult to figure out unless you look at the header or source files from the Palworld Modding Kit. You can also opt for trial and error as Pal Schema should output errors which tell you what type was expected for a property.

Key/Value could be anything in Unreal like a Struct, but most of the types are covered in the docs, so it should be no issue.