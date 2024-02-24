#include "r2/Game/ECS/System.h"

namespace r2::io
{
	class IInputGather;
	struct InputState;
	struct InputType;
}



struct PlayerCommandComponent;

class PlayerCommandSystem :public r2::ecs::System
{
public:
	PlayerCommandSystem();

	void SetInputGather(const r2::io::IInputGather* inputGather);
	virtual void Update() override;


private:

	void DoKeyboardMapping(const r2::io::InputState& inputState, const r2::io::InputType& inputType, PlayerCommandComponent& playerCommandComponent);
	void DoControllerMapping(const r2::io::InputState& inputState, const r2::io::InputType& inputType, PlayerCommandComponent& playerCommandComponent);

	const r2::io::IInputGather* mnoptrInputGather;
};