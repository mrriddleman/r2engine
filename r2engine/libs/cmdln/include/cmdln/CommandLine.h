#ifndef __COMMAND_LINE_H__
#define __COMMAND_LINE_H__

#include <string>
#include <vector>
#include <variant>

namespace r2::cmdln
{
	class CommandLine
	{
	public:

		using Value = std::variant<
			int32_t*,
			uint32_t*,
			double*,
			float*,
			bool*,
			std::string*>;

		explicit CommandLine(std::string description);

		void AddArgument(const std::vector<std::string>& flags, const Value& value, const std::string& help);

		void PrintHelp() const;

		void Parse(int argc, char* argv[]) const;

	private:
		struct Argument
		{
			std::vector<std::string> mFlags;
			Value mValue;
			std::string mHelp;
		};

		std::string mDescription;
		std::vector<Argument> mArguments;
	};
}


#endif // __COMMAND_LINE_H__
