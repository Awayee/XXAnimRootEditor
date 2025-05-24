#pragma once
#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "XXRemoveRootMotionModifer.generated.h"

UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EXXRemoveRootTranslation: uint8{
    None = 0 UMETA(Hidden),
    X = 1,
    Y = 2,
    Z = 4,
};
ENUM_CLASS_FLAGS(EXXRemoveRootTranslation);

UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EXXRemoveRootRotation : uint8 {
    None = 0 UMETA(Hidden),
    Roll = 1,
    Pitch = 2,
    Yaw = 4,
};
ENUM_CLASS_FLAGS(EXXRemoveRootRotation);


/** Remove root motion, while keep the final pose. */
UCLASS()
class XXANIMROOTEDITOR_API UXXRemoveRootMotionModifer: public UAnimationModifier {
	GENERATED_BODY()
public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (Bitmask, BitmaskEnum = "/Script/XXAnimRootEditor.EXXRemoveRootTranslation"));
    uint8 RemoveTranslation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (Bitmask, BitmaskEnum = "/Script/XXAnimRootEditor.EXXRemoveRootRotation"));
    uint8 RemoveRotation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings");
    int32 FrameStart;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings");
    int32 FrameEnd;

	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
};