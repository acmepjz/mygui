/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_MenuItem.h"
#include "MyGUI_WidgetManager.h"

namespace MyGUI
{

	MenuItem::MenuItem() :
		mOwner(nullptr),
		mMinSize(10, 10),
		mCheck(nullptr),
		mCheckValue(false)
	{
	}

	void MenuItem::initialiseOverride()
	{
		Base::initialiseOverride();

		// FIXME проверить смену скина ибо должно один раз вызываться
		Widget* parent = getParent();
		MYGUI_ASSERT(parent, "MenuItem must have parent MenuControl");
		if (!parent->isType<MenuControl>())
		{
			Widget* client = parent;
			parent = client->getParent();
			MYGUI_ASSERT(parent, "MenuItem must have parent MenuControl");
			MYGUI_ASSERT(parent->getClientWidget() == client, "MenuItem must have parent MenuControl");
			MYGUI_ASSERT(parent->isType<MenuControl>(), "MenuItem must have parent MenuControl");
		}
		mOwner = parent->castType<MenuControl>();

		///@wskin_child{MenuItem, Widget, Check} Галочка для отмеченного состояния.
		assignWidget(mCheck, "Check");

		if (isUserString("MinSize"))
			mMinSize = IntSize::parse(getUserString("MinSize"));

		if (isUserString("TextBoxSkin"))
			mTextBoxSkin = getUserString("TextBoxSkin");

		//FIXME нам нужен фокус клавы
		setNeedKeyFocus(true);

		updateCheck();
	}

	void MenuItem::shutdownOverride()
	{
		// FIXME проверить смену скина ибо должно один раз вызываться
		mOwner->_notifyDeleteItem(this);

		if (!mTextBox.empty()) {
			WidgetManager::getInstance().destroyWidgets(mTextBox);
			mTextBox.clear();
		}

		Base::shutdownOverride();
	}

	void MenuItem::onWidgetCreated(Widget* _widget)
	{
		Base::onWidgetCreated(_widget);

		MenuControl* child = _widget->castType<MenuControl>(false);
		if (child != nullptr)
		{
			mOwner->_wrapItemChild(this, child);
		}
	}

	void MenuItem::setCaption(const UString& _value) {
		mCaption = _value;
		_updateCaption();
		mOwner->_notifyUpdateCaption(this);
	}

	const UString& MenuItem::getCaption() {
		return mCaption;
	}

	void MenuItem::_updateCaption()
	{
		if (!mTextBox.empty()) {
			WidgetManager::getInstance().destroyWidgets(mTextBox);
			mTextBox.clear();
		}

		if (mTextBoxSkin.empty()) {
			Button::setCaption(mCaption);
		} else {
			size_t lps = 0;
			for (int i = -1;; i++) {
				size_t lpe = mCaption.find_first_of(UString::unicode_char('\t'), lps);
				UString s = mCaption.substr(lps, lpe == UString::npos ? UString::npos : (lpe - lps));

				if (i >= 0) {
					TextBox *textBox = createWidgetT("TextBox", mTextBoxSkin, IntCoord(), Align::Default)->castType<TextBox>();
					textBox->setTextAlign(getTextAlign());
					textBox->setNeedKeyFocus(false);
					textBox->setNeedMouseFocus(false);
					textBox->setCaption(s);
					mTextBox.push_back(textBox);
				} else {
					Button::setCaption(s);
				}

				if (lpe == UString::npos) break;
				lps = lpe + 1;
			}
		}
	}

	void MenuItem::_getTextSize2(std::vector<IntSize>& _size, IntSize& _size2) {
		_size.clear();

		ISubWidgetText *text = getSubWidgetText();
		if (text) {
			IntSize textSize = text->getTextSize();
			IntCoord textRegionSize = text->getCoord();
			IntSize widgetSize = getSize();

			_size.push_back(IntSize(textRegionSize.left + textSize.width,
				textSize.height + widgetSize.height - textRegionSize.height));
			_size2.set(widgetSize.width - textRegionSize.right(), mMinSize.height);
		} else {
			_size.push_back(mMinSize);
			_size2.set(0, 0);
		}

		for (int i = 0, m = mTextBox.size(); i < m; i++) {
			TextBox* _item = mTextBox[i]->castType<TextBox>();
			ISubWidgetText* text = _item->getSubWidgetText();
			if (text) {
				IntSize textSize = text->getTextSize();
				IntSize textRegionSize = text->getSize();
				IntSize widgetSize = _item->getSize();
				_size.push_back(textSize + widgetSize - textRegionSize);
			} else {
				_size.push_back(IntSize());
			}
			_size[i].width += 4; // FIXME: ad-hoc add some horizontal space
		}
	}

	void MenuItem::_setTextSize2(const std::vector<IntSize>& _size) {
		if (_size.empty()) return;

		int x = _size[0].width;
		int h = getHeight();

		for (size_t i = 0; i < mTextBox.size() && i + 1 < _size.size(); i++) {
			const IntSize& sz = _size[i + 1];
			mTextBox[i]->setCoord(x, 0, sz.width, h);
			x += sz.width;
		}
	}

	void MenuItem::setFontName(const std::string& _value)
	{
		Button::setFontName(_value);

		for (int i = 0, m = mTextBox.size(); i < m; i++) {
			mTextBox[i]->castType<TextBox>()->setFontName(_value);
		}

		mOwner->_notifyUpdateCaption(this);
	}

	void MenuItem::setFontHeight(int _value)
	{
		Button::setFontHeight(_value);

		for (int i = 0, m = mTextBox.size(); i < m; i++) {
			mTextBox[i]->castType<TextBox>()->setFontHeight(_value);
		}

		mOwner->_notifyUpdateCaption(this);
	}

	void MenuItem::setItemData(Any _data)
	{
		mOwner->setItemData(this, _data);
	}

	void MenuItem::removeItem()
	{
		mOwner->removeItem(this);
	}

	size_t MenuItem::getItemIndex()
	{
		return mOwner->getItemIndex(this);
	}

	MenuControl* MenuItem::createItemChild()
	{
		return mOwner->createItemChild(this);
	}

	void MenuItem::setItemType(MenuItemType _type)
	{
		mOwner->setItemType(this, _type);
	}

	MenuItemType MenuItem::getItemType()
	{
		return mOwner->getItemType(this);
	}

	void MenuItem::setItemChildVisible(bool _visible)
	{
		mOwner->setItemChildVisible(this, _visible);
	}

	MenuControl* MenuItem::getItemChild()
	{
		return mOwner->getItemChild(this);
	}

	void MenuItem::setPropertyOverride(const std::string& _key, const std::string& _value)
	{
		/// @wproperty{MenuItem, MenuItemType, MenuItemType} Тип строки меню.
		if (_key == "MenuItemType")
			setItemType(utility::parseValue<MenuItemType>(_value));

		/// @wproperty{MenuItem, MenuItemChecked, bool} Отмеченное состояние строки меню.
		else if (_key == "MenuItemChecked")
			setItemChecked(utility::parseValue<bool>(_value));

		else
		{
			Base::setPropertyOverride(_key, _value);
			return;
		}

		eventChangeProperty(this, _key, _value);
	}

	MenuControl* MenuItem::getMenuCtrlParent()
	{
		return mOwner;
	}

	IItemContainer* MenuItem::_getItemContainer()
	{
		return mOwner;
	}

	void MenuItem::updateCheck()
	{
		if (mCheck != nullptr)
			mCheck->setVisible(mCheckValue);
	}

	bool MenuItem::getItemChecked() const
	{
		return mCheckValue;
	}

	void MenuItem::setItemChecked(bool _value)
	{
		mCheckValue = _value;
		updateCheck();
	}

} // namespace MyGUI
