#ifndef EU4_CUSTOM_COLORS_H
#define EU4_CUSTOM_COLORS_H

#include "Color.h"

namespace EU4
{
	struct CustomColorsBlock
	{
		int flag = 0;
		int color = 0;
		int symbolIndex = 0;
		commonItems::Color flagColors = commonItems::Color(0, 0, 0);
	};

	class CustomColors : public  commonItems::parser
	{
	public:
		CustomColors() = default;
		explicit CustomColors(std::istream& theStream);
		[[nodiscard]] const auto& getCustomColors() const { return customColors; }

		void setFlag(const int fl) { customColors.flag = fl; }
		void setColor(const int col) { customColors.color = col; }
		void setSymbolIndex(const int si) { customColors.symbolIndex = si; }
		void setFlagColors(const commonItems::Color& col) { customColors.flagColors = col; }

	private:
		CustomColorsBlock customColors; // It walks like a color, it quacks like a color, but it's in fact a color INDEX from 00_custom_colors.txt
	};
}

#endif // EU4_CUSTOM_COLORS_H
