# Adding Ranch Suitability

:::warning
If you're interested in the legacy version of this guide, see the [legacy version](addingranchsuitability.md).
:::

So you've been wondering if you could make Tocotoco generate Gunpowder at the Ranch? Well, wonder no longer as we are finally able to do just that!

If you're new to PalSchema, I recommend checking out the [Getting Started](../../gettingstarted.md) guide first.

## Folder Structure

1. In your `PalSchema/mods` folder, create a new folder called `TocotocoRanch` which will be the name of our mod. Then inside it create the following folders:

- **blueprints**
- **pals**
- **raw**

You **MUST** name the three folders this way, otherwise PalSchema won't pick these up. The only cases where name doesn't matter is the name of your mod and any .json files you create.

## Raw Tables

1. Navigate to the `raw` folder and create a new file in there called `lottery.json`

2. `DT_ItemLotteryDataTable` is the table where the drops for Ranch pals are stored, so we'll want to add our entries to that table like so:

```json
{
  "DT_ItemLotteryDataTable": {
    "TocotocoRanch_001": {
      "FieldName": "CharacterSpawnItem_Tocotoco_1",
      "SlotNo": 1,
      "WeightInSlot": 1.0,
      "StaticItemId": "Gunpowder2",
      "MinNum": 1,
      "MaxNum": 1,
      "NumUnit": 1,
      "TreasureBoxGrade": "EPalMapObjectTreasureGradeType::Grade1"
    },
    "TocotocoRanch_002": {
      "FieldName": "CharacterSpawnItem_Tocotoco_2",
      "SlotNo": 1,
      "WeightInSlot": 1.0,
      "StaticItemId": "Gunpowder2",
      "MinNum": 1,
      "MaxNum": 2,
      "NumUnit": 1,
      "TreasureBoxGrade": "EPalMapObjectTreasureGradeType::Grade1"
    },
    "TocotocoRanch_003": {
      "FieldName": "CharacterSpawnItem_Tocotoco_3",
      "SlotNo": 1,
      "WeightInSlot": 1.0,
      "StaticItemId": "Gunpowder2",
      "MinNum": 1,
      "MaxNum": 3,
      "NumUnit": 1,
      "TreasureBoxGrade": "EPalMapObjectTreasureGradeType::Grade1"
    },
    "TocotocoRanch_004": {
      "FieldName": "CharacterSpawnItem_Tocotoco_4",
      "SlotNo": 1,
      "WeightInSlot": 1.0,
      "StaticItemId": "Gunpowder2",
      "MinNum": 1,
      "MaxNum": 4,
      "NumUnit": 1,
      "TreasureBoxGrade": "EPalMapObjectTreasureGradeType::Grade1"
    },
    "TocotocoRanch_005": {
      "FieldName": "CharacterSpawnItem_Tocotoco_5",
      "SlotNo": 1,
      "WeightInSlot": 1.0,
      "StaticItemId": "Gunpowder2",
      "MinNum": 1,
      "MaxNum": 5,
      "NumUnit": 1,
      "TreasureBoxGrade": "EPalMapObjectTreasureGradeType::Grade1"
    }
  }
}
```

You might notice that in the original table, all the row names are pure numbers. In the context of us adding ranch drops, absolutely do NOT use pure numbers yourself because you risk causing side effects like overwriting vanilla drops for other things in the game. `FieldName` is something you should try to remember as we'll be referencing it in a couple files.

You'll also notice that each `FieldName` ends with an underscore and a number. We'll cover this in the [Blueprint](#blueprint) section later.

3. Create a new file called `lottery_probability.json` inside the `raw` folder and copy the following inside the .json file:

```json
{
  "DT_FieldLotteryNameDataTable": {
    "CharacterSpawnItem_Tocotoco_1": {
      "ItemSlot1_ProbabilityPercent": 100.0
    },
    "CharacterSpawnItem_Tocotoco_2": {
      "ItemSlot1_ProbabilityPercent": 100.0
    },
    "CharacterSpawnItem_Tocotoco_3": {
      "ItemSlot1_ProbabilityPercent": 100.0
    },
    "CharacterSpawnItem_Tocotoco_4": {
      "ItemSlot1_ProbabilityPercent": 100.0
    },
    "CharacterSpawnItem_Tocotoco_5": {
      "ItemSlot1_ProbabilityPercent": 100.0
    }
  }
}
```

We'll be referencing the `FieldName` from our `lottery.json` file. We'll make `ItemSlot1_ProbabilityPercent` 100% in this guide which is the default for ranch pals, but you're free to make it 50.0 or whatever you want. We assigned `SlotNo` to be 1 in the `lottery.json` for all fields so that's why we're using `ItemSlot1_ProbabilityPercent` here.

## Blueprint

Next, we want to navigate to our `blueprints` folder we created earlier and create a `bp_tocotoco.json` file in which we will copy the following:

```json
{
  "BP_ColorfulBird_C": {
    "StaticCharacterParameterComponent": {
      "SpawnItem": {
        "FieldLotteryNameByRank": [
          {
            "Key": 1,
            "Value": {
              "Key": "CharacterSpawnItem_Tocotoco_1"
            }
          },
          {
            "Key": 2,
            "Value": {
              "Key": "CharacterSpawnItem_Tocotoco_2"
            }
          },
          {
            "Key": 3,
            "Value": {
              "Key": "CharacterSpawnItem_Tocotoco_3"
            }
          },
          {
            "Key": 4,
            "Value": {
              "Key": "CharacterSpawnItem_Tocotoco_4"
            }
          },
          {
            "Key": 5,
            "Value": {
              "Key": "CharacterSpawnItem_Tocotoco_5"
            }
          }
        ]
      }
    }
}
}
```

Now let's explain the properties in the blueprint:

- **StaticCharacterParameterComponent** - This is a component within every pal's blueprint which has a LOT of functionality in it.
  - **SpawnItem** - [Struct](../../types/structproperty.md) within `StaticCharacterParameterComponent` which contains the `FieldLotteryNameByRank` property.
    - **FieldLotteryNameByRank** - [Map](../../types/mapproperty.md) of integer/Struct(FName) pairs where the integer is Star Level of the pal - 1, for example a value 1 would be Star Level 0. The `Key` property inside `Value` references a `FieldName` entry inside `DT_ItemLotteryDataTable`.

## Pals Json

1. Lastly, we want to navigate to our `pals` folder we created earlier and create a `tocotoco.json` file in which we will copy the following:

```json
{
  "ColorfulBird": {
    "WorkSuitability_MonsterFarm": 1,
    "RanchActionData": {
      "ChargeMontage": "/Game/Pal/Animation/Character/Monster/ColorfulBird/AM_ColofulBird_FarSkill_ActionLoop.AM_ColofulBird_FarSkill_ActionLoop",
      "FunMontage": "/Game/Pal/Animation/Character/Monster/ColorfulBird/AM_ColofulBird_Damage.AM_ColofulBird_Damage",
      "ChargeFacialEye": "EPalFacialEyeType::Pain",
      "FunFacialEye": "EPalFacialEyeType::Smile",
      "SpawnSocketName": "tail_02"
    }
  }
}
```

Explanations for the properties in `RanchActionData`:

- **ChargeFacialEye** - This is the facial expression made by the pal when it's preparing to spawn an item.

- **ChargeMontage** - This is the animation loop that the pal will be performing as it is preparing to spawn an item.

- **FunFacialEye** - This is the facial expression made by the pal once it finishes spawning an item.

- **FunMontage** - This is the animation performed by the pal once it finishes spawning an item.

- **SpawnSocketName** - This is the name of the bone socket of the pal where the item will spawn at. There is no guarantee that every pal has a `tail_02` bone so you'll want to use something like FModel to inspect the skeleton of the pal and look up its list of bones e.g. if you wanted to check the bones for Nitewing, you'd look for the `SK_HawkBird_Skeleton` asset, open it and then search for `ReferenceSkeleton` and now you have a list of different bones.

Normally you would need to create a new `PalActionSpawnItem` blueprint that inherits from `BP_Action_SpawnItemBase` in the Palworld Modding Kit for this, but starting from 0.6.0, PalSchema will automatically create a new `BP_Action_SpawnItemBase` in the game's memory if you provide a `RanchActionData` entry in the pals json and you set `WorkSuitability_MonsterFarm` to 1 (or above which doesn't really do anything in the game currently).

## Finished Project

If we did everything correctly, Tocotoco should now have the ability to work at the Ranch!

![GIF showcasing the final results](assets/tutorial1/Final.gif)

You can do this with any pal you want, just change the references from `ColorfulBird` to your chosen pal. You can find a list of pal IDs on [Paldeck site](https://paldeck.cc/pals). Just click the little copy icon in the bottom-right corner of a pal card.

I recommend checking out the [CattivaRanchSuitability](https://github.com/Okaetsu/PalSchema/tree/main/assets/examples/CattivaRanchSuitability) example which also has comment explanations for pretty much every field.