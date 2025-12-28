---
sidebar_position: 2
---

# Wildcard

In Raw Tables, you have the ability to use wildcards to apply changes to every row of the table you're targeting. This can be a very useful alternative to just manually duplicating your change to every row.

Using wildcards is very simple, you just write a `*` instead of the row name, I'll leave an example below:

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