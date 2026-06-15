---
sidebar_position: 3
---

# FText

FText in Pal Schema will have some differences compared to FModel. `Pal/Content/L10N/en/Pal/DataTable/Text/DT_ItemNameText.uasset` will be used as an example.

Firstly let's look at how FModel displays an FText entry:
```json
"ITEM_NAME_Accessory_NormalResist_1": {
    "TextData": {
        "Namespace": "DT_ItemNameText_Common",
        "Key": "ITEM_NAME_Accessory_NormalResist_1_TextData",
        "SourceString": "Ring of Neutral Resistance",
        "LocalizedString": "Ring of Neutral Resistance"
    }
}
```

It's very verbose. In Pal Schema you would define it like this:
```json
"ITEM_NAME_Accessory_NormalResist_1": "Ring of Neutral Resistance"
```

That's all you need to do.