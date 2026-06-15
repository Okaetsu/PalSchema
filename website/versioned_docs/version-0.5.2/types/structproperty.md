---
sidebar_position: 9
---

# Struct

Struct Properties are a bit more complex as it will be combining a lot of the previous types. Struct Property is not something commonly seen in Datatables. You're more likely to see them when working with blueprints. `Pal/Content/Pal/DataTable/ItemShop/DT_ItemShopCreateData.uasset` will be used as an example.

`productDataArray` in `DT_ItemShopCreateData` is an [Array Property](./arrayproperty.md), but the items inside the array are Struct Properties. Below is how you would define a Struct Property in Pal Schema:

```json
{
    "StaticItemId": "Blueprint_Head003_1",
    "ProductType": "EPalItemShopProductType::Normal",
    "OverridePrice": 0,
    "ProductNum": 1,
    "Stock": 0
}
```

- `StaticItemId` is a [FName Property](./fname.md)
- `ProductType` is an [Enum Property](./enumproperty.md)
- `OverridePrice`, `ProductNum` and `Stock` are [Numeric Properties](./numericproperty.md)

As you can see we defined the struct as a json object.