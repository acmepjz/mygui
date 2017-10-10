/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_FONT_MANAGER_H_
#define MYGUI_FONT_MANAGER_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_Enumerator.h"
#include "MyGUI_IFont.h"
#include "MyGUI_Singleton.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_BackwardCompatibility.h"

namespace MyGUI
{

	class MYGUI_EXPORT FontManager :
		public Singleton<FontManager>,
		public MemberObsolete<FontManager>
	{
	public:
		FontManager();

		void initialise();
		void shutdown();

		/** Get default font name.
			Default skin also used when creating widget with skin that doesn't exist.
		*/
		const std::string& getDefaultFont() const;
		/** Get default font name.
			Default skin also used when creating widget with skin that doesn't exist.
		*/
		void setDefaultFont(const std::string& _value);

		/** Get font resource */
		IFont* getByName(const std::string& _name) const;

		/** Get invert selected text color property. */
		bool getInvertSelected() const { return mInvertSelect; }

		/** Enable or disable inverting color of selected text.\n
		Enabled (true) by default.
		*/
		void setInvertSelected(bool _value) { mInvertSelect = _value; }

		/** Get the global selected text color (used when getInvertSelected() == false). */
		uint32 getSelectedColour() const { return mSelectedColour; }

		/** Set the global selected text color (used when getInvertSelected() == false).\n
		Default value is 0xFFFFFFFF (white).
		*/
		void setSelectedColour(uint32 _value) { mSelectedColour = _value; }

	private:
		void _load(xml::ElementPtr _node, const std::string& _file, Version _version);

	private:
		std::string mDefaultName;
		bool mInvertSelect;
		uint32 mSelectedColour;

		bool mIsInitialise;
		std::string mXmlFontTagName;
		std::string mXmlPropertyTagName;
		std::string mXmlDefaultFontValue;
	};

} // namespace MyGUI

#endif // MYGUI_FONT_MANAGER_H_
