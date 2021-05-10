//https://schneegans.github.io/tutorials/2019/08/06/commandline

#include "cmdln/CommandLIne.h"

#include <sstream>
#include <algorithm>
#include <iomanip>

namespace r2::cmdln
{

	CommandLine::CommandLine(std::string description)
		:mDescription(std::move(description))
	{

	}

	void CommandLine::AddArgument(const std::vector<std::string>& flags, const Value& value, const std::string& help)
	{
		mArguments.emplace_back(Argument{flags, value, help});
	}

	void CommandLine::PrintHelp() const
	{
		printf("%s\n\n", mDescription.c_str());

		uint32_t maxFlagLength = 0;

		for (const auto& argument : mArguments)
		{
			uint32_t flagLength = 0;
			for (const auto& flag : argument.mFlags)
			{
				flagLength += static_cast<uint32_t>(flag.size()) + 2;
			}

			maxFlagLength = std::max(maxFlagLength, flagLength);
		}

		for (const auto& argument : mArguments)
		{
			std::string flags;

			for (const auto& flag : argument.mFlags)
			{
				flags += flag + ", ";
			}

			std::stringstream sstr;

			sstr << std::left << std::setw(maxFlagLength) << flags.substr(0, flags.size() - 2);


			size_t spacePos = 0;
			size_t lineWidth = 0;

			while (spacePos != std::string::npos)
			{
				size_t nextSpacePos = argument.mHelp.find_first_of(' ', spacePos + 1);
				sstr << argument.mHelp.substr(spacePos, nextSpacePos - spacePos);
				lineWidth += nextSpacePos - spacePos;
				spacePos = nextSpacePos;

				if (lineWidth > 60)
				{
					printf("%s\n", sstr.str().c_str());
					sstr = std::stringstream();
					sstr << std::left << std::setw(maxFlagLength - 1) << " ";
					lineWidth = 0;
				}
			}
		}
	}

	void CommandLine::Parse(int argc, char* argv[]) const
	{
		int i = 1; //skip the first argument which is the name of the program
		while (i < argc)
		{
			std::string flag(argv[i]);
			std::string value;
			bool        valueIsSeparate = false;

			// If there is an '=' in the flag, the part after the '=' is actually
			// the value.
			size_t equalPos = flag.find('=');
			if (equalPos != std::string::npos) {
				value = flag.substr(equalPos + 1);
				flag = flag.substr(0, equalPos);
			}
			// Else the following argument is the value.
			else if (i + 1 < argc) {
				value = argv[i + 1];
				valueIsSeparate = true;
			}

			// Search for an argument with the provided flag.
			bool foundArgument = false;

			for (auto const& argument : mArguments) {
				if (std::find(argument.mFlags.begin(), argument.mFlags.end(), flag)
					!= std::end(argument.mFlags)) {

					foundArgument = true;

					// In the case of booleans, there must not be a value present.
					// So if the value is neither 'true' nor 'false' it is considered
					// to be the next argument.
					if (std::holds_alternative<bool*>(argument.mValue)) {
						if (!value.empty() && value != "true" && value != "false") {
							valueIsSeparate = false;
						}
						*std::get<bool*>(argument.mValue) = (value != "false");
					}
					// In all other cases there must be a value.
					else if (value.empty()) {
						throw std::runtime_error(
							"Failed to parse command line arguments: "
							"Missing value for argument \"" + flag + "\"!");
					}
					// For a std::string, we take the entire value.
					else if (std::holds_alternative<std::string*>(argument.mValue)) {
						*std::get<std::string*>(argument.mValue) = value;
					}
					// In all other cases we use a std::stringstream to
					// convert the value.
					else {
						std::visit(
							[&value](auto&& arg) {
								std::stringstream sstr(value);
								sstr >> *arg;
							},
							argument.mValue);
					}

					break;
				}
			}

			// Print a warning if there was an unknown argument.
			if (!foundArgument) {
				printf("Ignoring unknown command line argument \"%s\".\n", flag.c_str());
			}

			// Advance to the next flag.
			++i;

			// If the value was separated, we have to advance our index once more.
			if (foundArgument && valueIsSeparate) {
				++i;
			}

		}
	}

}