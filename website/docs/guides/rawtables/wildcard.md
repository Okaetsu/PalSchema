---
sidebar_position: 2
---

# Raw Table Wildcards

In Raw Tables, you have the ability to use wildcards to apply changes to every row of the table you're targeting. This can be a very useful alternative to just manually duplicating your change to every row.

## Basic Example

Using wildcards is very simple, you just write a `*` instead of the row name:
```json
{
    "DT_PalMonsterParameter": {
        "*": {
            "WorkSuitability_EmitFlame": 5
        }
    }
}
```

Now every Pal should have maxed out Kindling work suitability, though it's a different story if they can utilize it! You can use wildcards for any data table and property.

*Wildcards can also be used to delete all entries from a table with the syntax `"*": null`.*
*This is useful for replacing an entire table with a new table by writing `"*": null` as the first entry.*
*Use caution with this technique, as clearing the entire table erases any changes made by other mods.*

## Filters

PalSchema 0.6.0 adds the ability to use filters with wildcards which gives you better control over which rows you want to modify in bulk.

### Example 1 (Single Filter)

We can use the code below to set the Kindling level of every Fire type pal to 1. `WorkSuitability_EmitFlame` is the internal name for Kindling.
```json
{
    "DT_PalMonsterParameter": {
        "*": {
            "$Filters": [
                {
                    "FieldName": "ElementType1",
                    "Operation": "Equal",
                    "Value": "Fire"
                }
            ],
            "WorkSuitability_EmitFlame": 1
        }
    }
}
```

`$Filters` holds an array of filters and all of the filters must match for your changes to be applied.

A filter holds the following fields:

* `FieldName` - Name of the property that you want to compare against.
* `Operation` - Operation that you want to perform on your chosen values, in this case Equal. [List of Operations](#operations) along with their aliases and supported property types.
* `Value` or `Values` - Value you want to compare the current row against. This can be `Value` if you only need to match against one value or it can be `Values` if you want to allow multiple values to pass. Note that the Boolean type doesn't support `Values` since a boolean can only be true or false.

### Example 2 (Multiple Filters)

This example sets the Hp of every building to 10000 with the material types of `Stone` and `Wood` as long as their `Hp` is below 10000.
```json
{
    "DT_MapObjectMasterDataTable": {
        "*": {
            "$Filters": [
                {
                    "FieldName": "MaterialType",
                    "Operation": "Equal",
                    "Values": ["Stone", "Wood"]
                },
                {
                    "FieldName": "Hp",
                    "Operation": "LessThan",
                    "Value": 10000
                }
            ],
            "Hp": 10000
        }
    }
}
```

In terms of code the above would translate to this:

```js
if ((CurrentRow.MaterialType == "Stone" or CurrentRow.MaterialType == "Wood") and CurrentRow.Hp < 10000)
    CurrentRow.Hp = 10000
```

Comparisons are done in A/B fashion where A is the current row being processed and B is the value you supplied to your filter.

### Operations

| Operation | Aliases | Supported Types |
|-----------|----------|----------------|
| Equals | `Equal`, `==` | FName, FString, FText, Bool, Numbers |
| GreaterThan | `>` | Numbers |
| GreaterThanOrEqual | `>=` | Numbers |
| LessThan | `<` | Numbers |
| LessThanOrEqual | `<=` | Numbers |
| Contains |  | FName, FString, FText |
| StartsWith |  | FName, FString, FText |
| EndsWith |  | FName, FString, FText |

### Supported Types

Only basic properties are supported as filters which are the following:

* FName, FString, FText
* Number
* Bool