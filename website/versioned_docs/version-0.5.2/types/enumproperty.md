---
sidebar_position: 5
---

# Enum

Enum Properties in Pal Schema are defined as strings. `Pal/Content/Pal/DataTable/Character/DT_PalMonsterParameter.uasset` will be used as an example.

You have two ways to specify an enum value, either specifying the full name:
```json
"Tribe": "EPalTribeID::PinkCat"
```

Or you can also do this where you leave the first part out:
```json
"Tribe": "PinkCat"
```