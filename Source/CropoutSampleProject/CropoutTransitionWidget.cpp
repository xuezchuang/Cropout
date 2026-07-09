// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutTransitionWidget.h"

#include "Animation/WidgetAnimation.h"
#include "TimerManager.h"

void UCropoutTransitionWidget::TransIn()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionOutRemovalTimer);
	}

	if (NewAnimation)
	{
		PlayAnimation(NewAnimation);
	}
}

void UCropoutTransitionWidget::TransOut()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionOutRemovalTimer);

		if (NewAnimation)
		{
			PlayAnimation(NewAnimation, 0.0f, 1, EUMGSequencePlayMode::Reverse);
			const float DelaySeconds = FMath::Max(0.0f, NewAnimation->GetEndTime());
			if (DelaySeconds > 0.0f)
			{
				World->GetTimerManager().SetTimer(
					TransitionOutRemovalTimer,
					this,
					&ThisClass::RemoveAfterTransitionOut,
					DelaySeconds,
					false);
				return;
			}
		}
	}

	RemoveAfterTransitionOut();
}

void UCropoutTransitionWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionOutRemovalTimer);
	}

	Super::NativeDestruct();
}

void UCropoutTransitionWidget::RemoveAfterTransitionOut()
{
	RemoveFromParent();
}
