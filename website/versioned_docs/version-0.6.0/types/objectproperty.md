# Object

Object properties in PalSchema are defined as strings that point to an UObject loaded in memory. PalSchema will load the UObject into memory if it's not loaded yet and it's a valid asset.

## Example

`Pal/Content/Pal/Blueprint/Action/Common/SpawnItem/BP_Action_SpawnItem_Alpaca.uasset`

`ChargeMontage` and `FunMontage` in `BP_Action_SpawnItem_Alpaca` are Object properties.

Below is an example of what an Object property looks like in FModel:
```json
"ChargeMontage": {
    "ObjectName": "AnimMontage'AM_Alpaca_FarSkill_ActionLoop'",
    "ObjectPath": "Pal/Content/Pal/Animation/Character/Monster/Alpaca/AM_Alpaca_FarSkill_ActionLoop.0"
}
```

You can see that it's a json object which has the fields `ObjectName` and `ObjectPath`.

In PalSchema you want to convert the `Pal/Content` in the beginning into `/Game/`. Make sure it has a forward slash in the beginning, otherwise your path will not be read correctly.

You want to combine the `ObjectName` and `ObjectPath` so that the final path becomes `/Game/Pal/Animation/Character/Monster/Alpaca/AM_Alpaca_FarSkill_ActionLoop.AM_Alpaca_FarSkill_ActionLoop`.

PalSchema Example:
```json
{
    "BP_Action_SpawnItem_Alpaca_C": {
        "ChargeMontage": "/Game/Pal/Animation/Character/Monster/Alpaca/AM_Alpaca_FarSkill_ActionLoop.AM_Alpaca_FarSkill_ActionLoop"
    }
}
```

As you can see, we just use a string right after `ChargeMontage`, rather than the complex object that can be seen in FModel.