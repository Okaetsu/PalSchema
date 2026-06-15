---
sidebar_position: 8
---

# TArray

Array Properties in Pal Schema have two ways of defining them. `Pal/Content/Pal/DataTable/ItemShop/DT_ItemShopCreateData.uasset` will be used as an example.

## Replacing the Array

`productDataArray` in `DT_ItemShopCreateData` is an Array Property. Let's say we want to overwrite the inventory of `Village_Shop_1`, we would do something like:

```json
"Village_Shop_1": {
    "productDataArray": [
        {
            "StaticItemId": "Pal_crystal_S",
            "ProductType": "EPalItemShopProductType::Normal",
            "OverridePrice": 0,
            "ProductNum": 1,
            "Stock": 0
        }
    ]
}
```

This will empty the existing array and replace it with our own, meaning that the only item in the Small Settlement shop will be Paldium Fragments (Pal_crystal_S).

## Inserting into the Array

If you wanted to append to the end of the array instead, you would specify the value as an object with an "Items" property inside, rather than as an array as seen below:

```json
"Village_Shop_1": {
    "productDataArray": {
        "Items": [
            {
                "StaticItemId": "Pal_crystal_S",
                "ProductType": "EPalItemShopProductType::Normal",
                "OverridePrice": 0,
                "ProductNum": 1,
                "Stock": 0
            }
        ]
    }
}
```

Now you've added Paldium Fragments to the Small Settlement shop along with other existing items.

`Items` field is not specific to this table, it is part of Pal Schema syntax which tells the code that these are the items you want to append to your array. So regardless of what array it is, always include the `Items` field in your array object if you're looking to insert into the array.