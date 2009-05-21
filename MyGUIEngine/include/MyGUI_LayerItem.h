/*!
	@file
	@author		Albert Semenov
	@date		02/2008
	@module
*/
/*
	This file is part of MyGUI.
	
	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __MYGUI_LAYER_ITEM_H__
#define __MYGUI_LAYER_ITEM_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Common.h"
#include "MyGUI_LayerManager.h"
#include "MyGUI_ILayer.h"
#include "MyGUI_ILayerNode.h"
#include "MyGUI_ILayerItem.h"

namespace MyGUI
{

	class MYGUI_EXPORT LayerItem : public ILayerItem
	{

	public:
		LayerItem() : mLayer(nullptr), mLayerNode(nullptr), mSaveLayerNode(nullptr) { }
		virtual ~LayerItem() { }

	public:
		ILayer* getLayer() { return mLayer; }

		virtual void attachItemToNode(ILayer* _layer, ILayerNode* _node);
		virtual void detachFromLayer();
		virtual void upLayerItem();

		void setRenderItemTexture(const std::string& _texture);

		void addChildItem(LayerItem* _item);
		void removeChildItem(LayerItem* _item);

		void addChildNode(LayerItem* _item);
		void removeChildNode(LayerItem* _item);

		void addRenderItem(IDrawItem* _item);
		void removeAllRenderItems();

		void saveLayerItem();
		void restoreLayerItem();

	protected:
		void attachToLayerItemKeeper(ILayerNode* _item, bool _deep);
		void detachFromLayerItemKeeper(bool _deep);

	private:
		// актуально для рутового виджета
		ILayer* mLayer;
		// конкретный айтем находящийся в слое
		ILayerNode * mLayerNode;
		ILayerNode * mSaveLayerNode;

		typedef std::vector<LayerItem*> VectorLayerItem;
		// список наших детей айтемов
		VectorLayerItem mLayerItems;
		// список наших узлов
		VectorLayerItem mLayerNodes;

		// вектор всех детей сабскинов
		VectorIDrawItem mDrawItems;

		std::string mTexture;

	};

} // namespace MyGUI

#endif // __MYGUI_LAYER_ITEM_H__
