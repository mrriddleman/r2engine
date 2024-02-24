#include "PlayerCommandSystem.h"
#include "r2/Core/Input/IInputGather.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Game/ECS/Components/PlayerComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/core/Input/InputState.h"
#include "PlayerCommandComponent.h"


PlayerCommandSystem::PlayerCommandSystem()
	:mnoptrInputGather(nullptr)
{

}

void PlayerCommandSystem::SetInputGather(const r2::io::IInputGather* inputGather)
{
	mnoptrInputGather = inputGather;
}

void PlayerCommandSystem::Update()
{
//	R2_CHECK(mnoptrInputGather != nullptr, "Should never happen");
	const r2::io::InputState& inputState = mnoptrInputGather->GetCurrentInputState();

	const auto numPlayerEntities = r2::sarr::Size(*mEntities);

	for (u32 i = 0; i < numPlayerEntities; ++i)
	{
		r2::ecs::Entity e = r2::sarr::At(*mEntities, i);
		const r2::ecs::PlayerComponent& playerComponent = mnoptrCoordinator->GetComponent<r2::ecs::PlayerComponent>(e);

		PlayerCommandComponent playerCommandComponent;
		playerCommandComponent.playerID = playerComponent.playerID;

		switch (playerComponent.inputType.inputType)
		{
		case  r2::io::INPUT_TYPE_KEYBOARD:
		{
			DoKeyboardMapping(inputState, playerComponent.inputType, playerCommandComponent);
		}
		break;
		case r2::io::INPUT_TYPE_CONTROLLER:
		{
			DoControllerMapping(inputState, playerComponent.inputType, playerCommandComponent);
		}
		break;
		default:
			R2_CHECK(false, "These are the only supported inputs for this game atm");
		}

		mnoptrCoordinator->AddComponent<PlayerCommandComponent>(e, playerCommandComponent);
	}
}

void PlayerCommandSystem::DoKeyboardMapping(const r2::io::InputState& inputState, const r2::io::InputType& inputType, PlayerCommandComponent& playerCommandComponent)
{
	//for now we'll just hardcode this
	//@TODO(Serge): probably we'll want to read the current keyboard mapping from somewhere (file, etc)

	playerCommandComponent.upAction.isEnabled = inputState.mKeyboardState.key_up.IsPressed();
	playerCommandComponent.upAction.enabledThisFrame = !inputState.mKeyboardState.key_up.repeated;

	playerCommandComponent.downAction.isEnabled = inputState.mKeyboardState.key_down.IsPressed();
	playerCommandComponent.downAction.enabledThisFrame = !inputState.mKeyboardState.key_down.repeated;

	playerCommandComponent.leftAction.isEnabled = inputState.mKeyboardState.key_left.IsPressed();
	playerCommandComponent.leftAction.enabledThisFrame = !inputState.mKeyboardState.key_left.repeated;

	playerCommandComponent.rightAction.isEnabled = inputState.mKeyboardState.key_right.IsPressed();
	playerCommandComponent.rightAction.enabledThisFrame = !inputState.mKeyboardState.key_right.repeated;

	playerCommandComponent.grabAction.isEnabled = inputState.mKeyboardState.key_a.IsPressed();
	playerCommandComponent.grabAction.enabledThisFrame = !inputState.mKeyboardState.key_a.repeated;
}

void PlayerCommandSystem::DoControllerMapping(const r2::io::InputState& inputState, const r2::io::InputType& inputType, PlayerCommandComponent& playerCommandComponent)
{
	//for now we'll just hardcode this
	//@TODO(Serge): probably we'll want to read the current keyboard mapping from somewhere (file, etc)

	playerCommandComponent.upAction.isEnabled = inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_DPAD_UP].IsPressed();
	playerCommandComponent.upAction.enabledThisFrame = !inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_DPAD_UP].held;

	playerCommandComponent.downAction.isEnabled = inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_DPAD_DOWN].IsPressed();
	playerCommandComponent.downAction.enabledThisFrame = !inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_DPAD_DOWN].held;

	playerCommandComponent.leftAction.isEnabled = inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_DPAD_LEFT].IsPressed();
	playerCommandComponent.leftAction.enabledThisFrame = !inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_DPAD_LEFT].held;

	playerCommandComponent.rightAction.isEnabled = inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_DPAD_RIGHT].IsPressed();
	playerCommandComponent.rightAction.enabledThisFrame = !inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_DPAD_RIGHT].held;

	playerCommandComponent.grabAction.isEnabled = inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_A].IsPressed();
	playerCommandComponent.grabAction.enabledThisFrame = !inputState.mControllersState[inputType.controllerID].buttons[r2::io::CONTROLLER_BUTTON_A].held;
}
