# Class

Class properties in PalSchema are defined as strings that point to an UClass loaded in memory. PalSchema will load the UClass into memory if it's not loaded yet and it's a valid asset.

## Example

`Pal/Content/Pal/Blueprint/Character/Funnel/BP_FunnelCharacter_DreamDemon.uasset`

`FunnelSkillAIActionClass` and `FunnelSkillModuleClass` in `BP_FunnelCharacter_DreamDemon` are Class properties.

Below is an example of what a Class property looks like in FModel:
```json
"FunnelSkillAIActionClass": {
    "ObjectName": "BlueprintGeneratedClass'BP_AIActionFunnelSkill_ShadowBall_C'",
    "ObjectPath": "Pal/Content/Pal/Blueprint/Funnel/ReticleTargetAttack/BP_AIActionFunnelSkill_ShadowBall.0"
}
```

You can see that it looks similar to the [Object](./objectproperty.md) property, but if you look at the end of `ObjectName`, you can see a `_C` which indicates that it's referring to a class.

In PalSchema you want to convert the `Pal/Content` in the beginning into `/Game/`. Make sure it has a forward slash in the beginning, otherwise your path will not be read correctly.

You want to combine the `ObjectName` and `ObjectPath` so that the final path becomes `/Game/Pal/Blueprint/Funnel/ReticleTargetAttack/BP_AIActionFunnelSkill_ShadowBall.BP_AIActionFunnelSkill_ShadowBall_C`.

Remember to always end your path in `_C` when dealing with any Class properties in PalSchema.

PalSchema Example:
```json
{
    "BP_FunnelCharacter_DreamDemon_C": {
        "FunnelSkillAIActionClass": "/Game/Pal/Blueprint/Funnel/ReticleTargetAttack/BP_AIActionFunnelSkill_ShadowBall.BP_AIActionFunnelSkill_ShadowBall_C"
    }
}
```

As you can see, we just use a string right after `FunnelSkillAIActionClass`, rather than the complex object that can be seen in FModel.