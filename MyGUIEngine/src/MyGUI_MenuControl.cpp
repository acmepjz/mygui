/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_MenuControl.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_MenuItem.h"
#include "MyGUI_ImageBox.h"
#include "MyGUI_MenuBar.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_LayerManager.h"
#include "MyGUI_ControllerManager.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_Gui.h"
#include "MyGUI_RenderManager.h"

namespace MyGUI
{

	const float POPUP_MENU_SPEED_COEF = 3.0f;

	MenuControl::MenuControl() :
		mHideByAccept(true),
		mMenuDropMode(false),
		mIsMenuDrop(true),
		mHideByLostKey(false),
		mResizeToContent(true),
		mShutdown(false),
		mFrameAdvise(false),
		mVerticalAlignment(true),
		mManualSkin(false),
		mDistanceButton(0),
		mPopupAccept(false),
		mOwner(nullptr),
		mAnimateSmooth(false),
		mChangeChildSkin(false),
		mInternalCreateChild(false)
	{
	}

	void MenuControl::initialiseOverride()
	{
		Base::initialiseOverride();

		// инициализируем овнера
		Widget* parent = getParent();
		if (parent)
		{
			mOwner = parent->castType<MenuItem>(false);
			if (!mOwner)
			{
				Widget* client = parent;
				parent = client->getParent();
				if (parent && parent->getClientWidget())
				{
					mOwner = parent->castType<MenuItem>(false);
				}
			}
		}

		// FIXME нам нужен фокус клавы
		setNeedKeyFocus(true);

		//OBSOLETE
		if (isUserString("SkinLine"))
		{
			mItemNormalSkin = getUserString("SkinLine");
			mItemPopupSkin = mItemNormalSkin;
		}

		if (isUserString("SeparatorSkin"))
			mItemSeparatorSkin = getUserString("SeparatorSkin");

		if (isUserString("NormalSkin"))
			mItemNormalSkin = getUserString("NormalSkin");

		if (isUserString("PopupSkin"))
			mItemPopupSkin = getUserString("PopupSkin");

		if (isUserString("DistanceButton"))
			mDistanceButton = utility::parseValue<int>(getUserString("DistanceButton"));

		if (isUserString("SubMenuSkin"))
			mSubMenuSkin = getUserString("SubMenuSkin");

		if (isUserString("SubMenuLayer"))
			mSubMenuLayer = getUserString("SubMenuLayer");

		// FIXME добавленно, так как шетдаун вызывается и при смене скина
		mShutdown = false;
	}

	void MenuControl::shutdownOverride()
	{
		mShutdown = true;

		if (mOwner != nullptr)
			mOwner->getMenuCtrlParent()->_notifyDeletePopup(mOwner);

		Base::shutdownOverride();
	}

	void MenuControl::onWidgetCreated(Widget* _widget)
	{
		Base::onWidgetCreated(_widget);

		MenuItem* child = _widget->castType<MenuItem>(false);
		if (child != nullptr && !mInternalCreateChild)
		{
			_wrapItem(child, mItemsInfo.size(), MenuItemType::Normal, Any::Null);
		}
	}

	MenuItem* MenuControl::insertItemAt(size_t _index, const UString& _caption, MenuItemType _type, const std::string& _name, Any _data)
	{
		MYGUI_ASSERT_RANGE_INSERT(_index, mItemsInfo.size(), "MenuControl::insertItemAt");
		if (_index == ITEM_NONE) _index = mItemsInfo.size();

		mInternalCreateChild = true;
		MenuItem* item = _getClientWidget()->createWidget<MenuItem>(getSkinByType(_type), IntCoord(), Align::Default, _name);
		item->setCaption(_caption);
		mInternalCreateChild = false;
		_wrapItem(item, _index, _type, _data);

		return item;
	}

	void MenuControl::removeItemAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::removeItemAt");

		if (mItemsInfo[_index].submenu)
		{
			WidgetManager::getInstance().destroyWidget(mItemsInfo[_index].submenu);
			mItemsInfo[_index].submenu = nullptr;
		}
		WidgetManager::getInstance().destroyWidget(mItemsInfo[_index].item);
	}

	void MenuControl::removeAllItems()
	{
		while (!mItemsInfo.empty())
		{
			if (mItemsInfo.back().submenu)
			{
				WidgetManager::getInstance().destroyWidget(mItemsInfo.back().submenu);
				mItemsInfo.back().submenu = nullptr;
			}
			WidgetManager::getInstance().destroyWidget(mItemsInfo.back().item);
		}
	}

	void MenuControl::update()
	{
		IntSize size;

		if (mVerticalAlignment)
		{
			std::vector<IntSize> sz1m;
			IntSize sz2m;
			std::vector<int> heights;

			// calculate size for each item
			for (size_t i = 0; i < mItemsInfo.size(); i++)
			{
				std::vector<IntSize> sz1;
				IntSize sz2;

				mItemsInfo[i].item->_getTextSize2(sz1, sz2);

				int height = sz2.height;
				for (size_t j = 0; j < sz1.size(); j++) {
					if (height < sz1[j].height) height = sz1[j].height;
				}
				heights.push_back(height);

				while (sz1m.size() < sz1.size()) sz1m.push_back(IntSize());
				for (size_t j = 0; j < sz1.size(); j++) {
					if (sz1m[j].width < sz1[j].width) sz1m[j].width = sz1[j].width;
				}
				if (sz2m.width < sz2.width) sz2m.width = sz2.width;
			}

			// calculate width
			size.width = sz2m.width;
			for (size_t j = 0; j < sz1m.size(); j++) size.width += sz1m[j].width;

			// resize each item
			for (size_t i = 0; i < mItemsInfo.size(); i++)
			{
				const int height = heights[i];

				mItemsInfo[i].item->setCoord(0, size.height, _getClientWidget()->getWidth(), height);
				size.height += height + mDistanceButton;

				mItemsInfo[i].item->_setTextSize2(sz1m);
			}

			if (!mItemsInfo.empty())
				size.height -= mDistanceButton;
		}
		else
		{
			std::vector<std::vector<IntSize> > sz1m;
			std::vector<IntSize> sz2m;

			// calculate size for each item
			for (size_t i = 0; i < mItemsInfo.size(); i++)
			{
				sz1m.push_back(std::vector<IntSize>());
				sz2m.push_back(IntSize());

				std::vector<IntSize> &sz1 = sz1m.back();
				IntSize &sz2 = sz2m.back();

				mItemsInfo[i].item->_getTextSize2(sz1, sz2);

				if (size.height < sz2.height) size.height = sz2.height;
				for (size_t j = 0; j < sz1.size(); j++) {
					if (size.height < sz1[j].height) size.height = sz1[j].height;
				}
			}

			// resize each item
			for (size_t i = 0; i < mItemsInfo.size(); i++)
			{
				std::vector<IntSize> &sz1 = sz1m[i];
				const IntSize &sz2 = sz2m[i];

				int width = sz2.width;
				for (size_t j = 0; j < sz1.size(); j++) width += sz1[j].width;

				mItemsInfo[i].item->setCoord(size.width, 0, width, size.height);
				size.width += width + mDistanceButton;

				mItemsInfo[i].item->_setTextSize2(sz1);
			}

			if (!mItemsInfo.empty())
				size.width -= mDistanceButton;
		}

		if (mResizeToContent)
			setSize(size + mCoord.size() - _getClientWidget()->getSize());
	}

	void MenuControl::setItemDataAt(size_t _index, Any _data)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::setItemDataAt");
		mItemsInfo[_index].data = _data;
	}

	MenuControl* MenuControl::getItemChildAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::getItemChildAt");
		return mItemsInfo[_index].submenu;
	}

	void MenuControl::removeItemChildAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::removeItemChildAt");

		if (mItemsInfo[_index].submenu != nullptr)
		{
			WidgetManager::getInstance().destroyWidget(mItemsInfo[_index].submenu);
			mItemsInfo[_index].submenu = nullptr;
		}

		frameAdvise(true);
	}

	const UString& MenuControl::getItemCaptionAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::getItemNameAt");
		return mItemsInfo[_index].item->getCaption();
	}

	void MenuControl::setItemCaptionAt(size_t _index, const UString& _caption)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::setItemNameAt");

		mItemsInfo[_index].item->setCaption(_caption);

		frameAdvise(true);
	}

	void MenuControl::_notifyDeleteItem(MenuItem* _item)
	{
		// дитю меняем скин
		if (mChangeChildSkin)
			return;

		// общий шутдаун виджета
		if (mShutdown)
			return;

		size_t index = getItemIndex(_item);
		mItemsInfo.erase(mItemsInfo.begin() + index);
		frameAdvise(true);
	}

	void MenuControl::_notifyDeletePopup(MenuItem* _item)
	{
		size_t index = getItemIndex(_item);
		mItemsInfo[index].submenu = nullptr;
	}

	void MenuControl::_notifyUpdateCaption(MenuItem* _item)
	{
		frameAdvise(true);
	}

	MenuItemType MenuControl::getItemTypeAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::getItemTypeAt");
		return mItemsInfo[_index].type;
	}

	void MenuControl::setItemTypeAt(size_t _index, MenuItemType _type)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::setItemTypeAt");
		ItemInfo& info = mItemsInfo[_index];
		if (info.type == _type)
			return;

		// сохраняем данные
		info.type = _type;

		// при смене скина дите отпишется
		if (!mManualSkin) {
			mChangeChildSkin = true;
			info.item->changeWidgetSkin(getSkinByType(_type));
			mChangeChildSkin = false;
		}

		info.item->setImageName(getIconIndexByType(_type ));
		info.item->_updateCaption();

		frameAdvise(true);
	}

	void MenuControl::changeItemSkinAt(size_t _index, const std::string& _skinName) {
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::changeItemSkinAt");
		ItemInfo& info = mItemsInfo[_index];

		mChangeChildSkin = true;
		info.item->changeWidgetSkin(_skinName);
		mChangeChildSkin = false;

		info.item->_updateCaption();

		frameAdvise(true);
	}

	void MenuControl::notifyMenuCtrlAccept(MenuItem* _item)
	{
		if (mHideByAccept)
		{
			setVisibleSmooth(false);
		}
		else
		{
			InputManager::getInstance().setKeyFocusWidget(nullptr);
		}

		MenuItem* parent_item = getMenuItemParent();
		if (parent_item)
		{
			MenuControl* parent_ctrl = parent_item->getMenuCtrlParent();
			if (parent_ctrl)
			{
				parent_ctrl->notifyMenuCtrlAccept(_item);
			}
		}

		eventMenuCtrlAccept(this, _item);
	}

	void MenuControl::setItemChildVisibleAt(size_t _index, bool _visible)
	{
		_setItemChildVisibleAt(_index, _visible, true);
	}

	void MenuControl::_setItemChildVisibleAt(size_t _index, bool _visible, bool _smooth)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::setItemChildVisibleAt");

		if (_visible)
		{
			if (mItemsInfo[_index].submenu && mItemsInfo[_index].submenu->getItemCount())
			{
				int offset = mItemsInfo[0].item->getAbsoluteTop() - getAbsoluteTop();

				const IntCoord& coord = mItemsInfo[_index].item->getAbsoluteCoord();
				IntPoint point(getAbsoluteRect().right, coord.top - offset);

				MenuControl* menu = mItemsInfo[_index].submenu;

				if (mVerticalAlignment)
				{
					// too wide
					if (point.left + menu->getWidth() > menu->getParentSize().width)
					{
						// move to the left side if possible
						if (point.left - menu->getWidth() - getWidth() > 0)
							point.left -= menu->getWidth() + getWidth();
						// or put near right parent border (window) if too wide for left side too
						else
							point.left = menu->getParentSize().width - menu->getWidth();
					}
					// too high (same logic as for too wide)
					if (point.top + menu->getHeight() > menu->getParentSize().height)
					{
						// move to the top side if possible
						if (point.top - menu->getHeight() - getHeight() > 0)
							point.top -= menu->getHeight() + getHeight();
						// or put near bottom parent border (window) if too high for top side too
						else
							point.top = menu->getParentSize().height - menu->getHeight();
					}
				}
				else
				{
					point.set(coord.left, getAbsoluteRect().bottom);
				}

				menu->setPosition(point);
				if (_smooth)
					menu->setVisibleSmooth(true);
				else
					menu->setVisible(true);

				MyGUI::LayerManager::getInstance().upLayerItem(menu);
			}
		}
		else
		{
			if (mItemsInfo[_index].submenu)
			{
				if (_smooth)
					mItemsInfo[_index].submenu->setVisibleSmooth(false);
				else
					mItemsInfo[_index].submenu->setVisible(false);
			}
		}
	}

	void MenuControl::notifyRootKeyChangeFocus(Widget* _sender, bool _focus)
	{
		MenuItem* item = _sender->castType<MenuItem>();
		if (item->getItemType() == MenuItemType::Popup)
		{
			if (_focus)
			{
				if (!mMenuDropMode || mIsMenuDrop)
				{
					item->setItemChildVisible(true);
					item->setStateSelected(true);
				}
			}
			else
			{
				item->setItemChildVisible(false);
				item->setStateSelected(false);
			}
		}
	}

	Widget* MenuControl::createItemChildByType(size_t _index, const std::string& _type)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::createItemChildByType");
		removeItemChildAt(_index);
		Widget* child = mItemsInfo[_index].item->createWidgetT(WidgetStyle::Popup, _type, mSubMenuSkin, IntCoord(), Align::Default, mSubMenuLayer);
		MYGUI_ASSERT(child->isType<MenuControl>(), "child must have MenuControl base type");
		return child;
	}

	void MenuControl::notifyMouseButtonClick(Widget* _sender)
	{
		MenuItem* item = _sender->castType<MenuItem>();
		if (mMenuDropMode)
		{
			if (mIsMenuDrop)
			{
				if (item->getItemType() == MenuItemType::Popup)
				{
					item->setStateSelected(false);
					item->setItemChildVisible(false);
					mIsMenuDrop = false;
				}
			}
			else
			{
				if (item->getItemType() == MenuItemType::Popup)
				{
					mIsMenuDrop = true;
					item->setStateSelected(true);
					item->setItemChildVisible(true);
					InputManager::getInstance().setKeyFocusWidget(item);
				}
			}
		}
		else
		{
			if ((item->getItemType() == MenuItemType::Popup && mPopupAccept) ||
				item->getItemType() == MenuItemType::Normal)
			{
				notifyMenuCtrlAccept(item);
			}
		}
	}

	void MenuControl::onKeyChangeRootFocus(bool _focus)
	{
		if (mMenuDropMode)
		{
			mIsMenuDrop = false;
		}
		if (!_focus && mHideByLostKey)
		{
			setVisibleSmooth(false);
			eventMenuCtrlClose(this);
		}
		Base::onKeyChangeRootFocus(_focus);
	}

	void MenuControl::notifyMouseSetFocus(Widget* _sender, Widget* _new)
	{
		InputManager::getInstance().setKeyFocusWidget(_sender);
	}

	void MenuControl::_wrapItemChild(MenuItem* _item, MenuControl* _widget)
	{
		// заменяем
		size_t index = getItemIndex(_item);
		if (mItemsInfo[index].submenu != nullptr)
		{
			WidgetManager::getInstance().destroyWidget(mItemsInfo[index].submenu);
			mItemsInfo[index].submenu = nullptr;
		}
		mItemsInfo[index].submenu = _widget;
		// скрываем менюшку
		mItemsInfo[index].submenu->setVisible(false);

		frameAdvise(true);
	}

	void MenuControl::_wrapItem(MenuItem* _item, size_t _index, MenuItemType _type, Any _data)
	{
		_item->setAlign(mVerticalAlignment ? Align::Top | Align::HStretch : Align::Default);
		_item->eventRootKeyChangeFocus += newDelegate(this, &MenuControl::notifyRootKeyChangeFocus);
		_item->eventMouseButtonClick += newDelegate(this, &MenuControl::notifyMouseButtonClick);
		_item->eventMouseSetFocus += newDelegate(this, &MenuControl::notifyMouseSetFocus);

		_item->setImageName(getIconIndexByType(_type ));

		MenuControl* submenu = nullptr;

		ItemInfo info = ItemInfo(_item, _type, submenu, _data);

		mItemsInfo.insert(mItemsInfo.begin() + _index, info);

		if (!mManualSkin) {
			mChangeChildSkin = true;
			_item->changeWidgetSkin(getSkinByType(_type));
			mChangeChildSkin = false;
		}

		// его сет капшен, обновит размер
		_item->_updateCaption();

		frameAdvise(true);
	}

	void MenuControl::setVisible(bool _visible)
	{
		if (mAnimateSmooth)
		{
			ControllerManager::getInstance().removeItem(this);
			setAlpha(ALPHA_MAX);
			setEnabledSilent(true);
			mAnimateSmooth = false;
		}

		if (_visible)
		{
			if (mOwner == nullptr && mHideByLostKey)
			{
				MyGUI::InputManager::getInstance().setKeyFocusWidget(this);
			}
		}

		Base::setVisible(_visible);
	}

	void MenuControl::setVisibleSmooth(bool _visible)
	{
		mAnimateSmooth = true;
		ControllerManager::getInstance().removeItem(this);

		if (_visible)
		{
			setEnabledSilent(true);
			if (!getVisible())
			{
				setAlpha(ALPHA_MIN);
				Base::setVisible(true);
			}

			ControllerFadeAlpha* controller = createControllerFadeAlpha(ALPHA_MAX, POPUP_MENU_SPEED_COEF, true);
			controller->eventPostAction += newDelegate(action::actionWidgetShow);
			ControllerManager::getInstance().addItem(this, controller);
		}
		else
		{
			setEnabledSilent(false);

			ControllerFadeAlpha* controller = createControllerFadeAlpha(ALPHA_MIN, POPUP_MENU_SPEED_COEF, false);
			controller->eventPostAction += newDelegate(action::actionWidgetHide);
			ControllerManager::getInstance().addItem(this, controller);
		}
	}

	ControllerFadeAlpha* MenuControl::createControllerFadeAlpha(float _alpha, float _coef, bool _enable)
	{
		ControllerItem* item = ControllerManager::getInstance().createItem(ControllerFadeAlpha::getClassTypeName());
		ControllerFadeAlpha* controller = item->castType<ControllerFadeAlpha>();

		controller->setAlpha(_alpha);
		controller->setCoef(_coef);
		controller->setEnabled(_enable);

		return controller;
	}

	void MenuControl::removeItem(MenuItem* _item)
	{
		removeItemAt(getItemIndex(_item));
	}

	MenuItem* MenuControl::getItemAt(size_t _index)
	{
		MYGUI_ASSERT_RANGE(_index, mItemsInfo.size(), "MenuControl::getItemAt");
		return mItemsInfo[_index].item;
	}

	size_t MenuControl::getItemIndex(MenuItem* _item)
	{
		for (size_t pos = 0; pos < mItemsInfo.size(); pos++)
		{
			if (mItemsInfo[pos].item == _item)
				return pos;
		}
		MYGUI_EXCEPT("item (" << _item << ") not found, source 'MenuControl::getItemIndex'");
	}

	size_t MenuControl::findItemIndex(MenuItem* _item)
	{
		for (size_t index = 0; index < mItemsInfo.size(); index++)
		{
			if (mItemsInfo[index].item == _item)
				return index;
		}
		return ITEM_NONE;
	}

	size_t MenuControl::getItemCount() const
	{
		return mItemsInfo.size();
	}

	void MenuControl::setItemData(MenuItem* _item, Any _data)
	{
		setItemDataAt(getItemIndex(_item), _data);
	}

	void MenuControl::clearItemDataAt(size_t _index)
	{
		setItemDataAt(_index, Any::Null);
	}

	void MenuControl::clearItemData(MenuItem* _item)
	{
		clearItemDataAt(getItemIndex(_item));
	}

	void MenuControl::setItemCaption(MenuItem* _item, const UString& _caption)
	{
		setItemCaptionAt(getItemIndex(_item), _caption);
	}

	const UString& MenuControl::getItemCaption(MenuItem* _item)
	{
		return getItemCaptionAt(getItemIndex(_item));
	}

	void MenuControl::setItemChildVisible(MenuItem* _item, bool _visible)
	{
		setItemChildVisibleAt(getItemIndex(_item), _visible);
	}

	MenuControl* MenuControl::getItemChild(MenuItem* _item)
	{
		return getItemChildAt(getItemIndex(_item));
	}

	MenuControl* MenuControl::createItemChildAt(size_t _index)
	{
		return createItemChildTAt<MenuControl>(_index);
	}

	MenuControl* MenuControl::createItemChild(MenuItem* _item)
	{
		return createItemChildAt(getItemIndex(_item));
	}

	void MenuControl::removeItemChild(MenuItem* _item)
	{
		removeItemChildAt(getItemIndex(_item));
	}

	MenuItemType MenuControl::getItemType(MenuItem* _item)
	{
		return getItemTypeAt(getItemIndex(_item));
	}

	void MenuControl::setItemType(MenuItem* _item, MenuItemType _type)
	{
		setItemTypeAt(getItemIndex(_item), _type);
	}

	void MenuControl::setPopupAccept(bool _value)
	{
		mPopupAccept = _value;
	}

	bool MenuControl::getPopupAccept() const
	{
		return mPopupAccept;
	}

	MenuItem* MenuControl::getMenuItemParent()
	{
		return mOwner;
	}

	const std::string& MenuControl::getSkinByType(MenuItemType _type) const
	{
		if (_type == MenuItemType::Popup)
			return mItemPopupSkin;
		else if (_type == MenuItemType::Separator)
			return mItemSeparatorSkin;
		return mItemNormalSkin;
	}

	std::string MenuControl::getIconIndexByType(MenuItemType _type) const
	{
		if (_type == MenuItemType::Popup)
			return "Popup";
		return "None";
	}

	MenuItemType MenuControl::getItemType(bool _submenu, bool _separator) const
	{
		if (_submenu)
			return MenuItemType::Popup;
		else if (_separator)
			return MenuItemType::Separator;
		return MenuItemType::Normal;
	}

	size_t MenuControl::_getItemCount()
	{
		return getItemCount();
	}

	void MenuControl::_addItem(const MyGUI::UString& _name)
	{
		addItem(_name, MenuItemType::Normal);
	}

	void MenuControl::_removeItemAt(size_t _index)
	{
		removeItemAt(_index);

		_updateSizeForEmpty();
	}

	Widget* MenuControl::_getItemAt(size_t _index)
	{
		return getItemAt(_index);
	}

	void MenuControl::_setItemSelected(IItem* _item)
	{
		MenuItem* item = static_cast<MenuItem*>(_item);
		for (VectorMenuItemInfo::iterator iter = mItemsInfo.begin(); iter != mItemsInfo.end(); ++iter)
		{
			if ((*iter).type == MenuItemType::Popup)
			{
				(*iter).item->setStateSelected(false);

				if ((*iter).submenu != nullptr)
					(*iter).submenu->setVisible(false);
			}
		}

		if (item->getItemType() == MenuItemType::Popup)
		{
			item->setStateSelected(true);
			size_t index = getItemIndex(item);

			_setItemChildVisibleAt(index, true, false);

			_updateItems(index);
		}
	}

	void MenuControl::_updateItems(size_t _index)
	{
		if (mItemsInfo[_index].submenu != nullptr)
			mItemsInfo[_index].submenu->_updateSizeForEmpty();
	}

	void MenuControl::_updateSizeForEmpty()
	{
		if (mItemsInfo.empty())
			setSize(100, 100);
	}

	void MenuControl::setVerticalAlignment(bool _value)
	{
		mVerticalAlignment = _value;

		frameAdvise(true);
	}

	bool MenuControl::getVerticalAlignment() const
	{
		return mVerticalAlignment;
	}

	void MenuControl::setPropertyOverride(const std::string& _key, const std::string& _value)
	{
		/// @wproperty{MenuControl, VerticalAlignment, bool} Вертикальное выравнивание.
		if (_key == "VerticalAlignment")
			setVerticalAlignment(utility::parseValue<bool>(_value));

		else if (_key == "ManualSkin")
			setManualSkin(utility::parseValue<bool>(_value));

		else
		{
			Base::setPropertyOverride(_key, _value);
			return;
		}

		eventChangeProperty(this, _key, _value);
	}

	void MenuControl::frameEntered(float _frame)
	{
		update();

		frameAdvise(false);
	}

	void MenuControl::frameAdvise(bool _advise)
	{
		if (_advise) {
			if (!mFrameAdvise) {
				MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(this, &MenuControl::frameEntered);
				mFrameAdvise = true;
			}
		} else {
			if (mFrameAdvise) {
				MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate(this, &MenuControl::frameEntered);
				mFrameAdvise = false;
			}
		}
	}

} // namespace MyGUI
