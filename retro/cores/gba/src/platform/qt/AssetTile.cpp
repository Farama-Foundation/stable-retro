/* Copyright (c) 2013-2016 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "AssetTile.h"

#include "CoreController.h"
#include "GBAApp.h"

#include <QFontDatabase>
#include <QHBoxLayout>

#include <mgba/core/interface.h>
#ifdef M_CORE_GBA
#include <mgba/internal/gba/memory.h>
#endif
#ifdef M_CORE_GB
#include <mgba/internal/gb/memory.h>
#endif

using namespace QGBA;

AssetTile::AssetTile(QWidget* parent)
	: QGroupBox(parent)
{
	m_ui.setupUi(this);

	m_ui.preview->setDimensions(QSize(8, 8));
	m_ui.color->setDimensions(QSize(1, 1));
	m_ui.color->setSize(50);

	connect(m_ui.preview, &Swatch::indexPressed, this, &AssetTile::selectColor);

	const QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

	m_ui.tileId->setFont(font);
	m_ui.paletteId->setFont(font);
	m_ui.address->setFont(font);
	m_ui.r->setFont(font);
	m_ui.g->setFont(font);
	m_ui.b->setFont(font);
}

void AssetTile::addCustomProperty(const QString& id, const QString& visibleName) {
	QHBoxLayout* newLayout = new QHBoxLayout;
	newLayout->addWidget(new QLabel(visibleName));
	QLabel* value = new QLabel;
	value->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	value->setAlignment(Qt::AlignRight);
	newLayout->addWidget(value);
	m_customProperties[id] = value;
	int index = layout()->indexOf(m_ui.line);
	static_cast<QBoxLayout*>(layout())->insertLayout(index, newLayout);
}

void AssetTile::setController(std::shared_ptr<CoreController> controller) {
	m_cacheSet = controller->graphicCaches();
	switch (controller->platform()) {
#ifdef M_CORE_GBA
	case PLATFORM_GBA:
		m_addressWidth = 8;
		m_addressBase = BASE_VRAM;
		m_boundaryBase = BASE_VRAM | 0x10000;
		break;
#endif
#ifdef M_CORE_GB
	case PLATFORM_GB:
		m_addressWidth = 4;
		m_addressBase = GB_BASE_VRAM;
		m_boundaryBase = GB_BASE_VRAM;
		break;
#endif
	default:
		m_addressWidth = 0;
		m_addressBase = 0;
		m_boundaryBase = 0;
		break;
	}
}

void AssetTile::setPalette(int palette) {
	m_paletteId = palette;
	selectIndex(m_index);
}

void AssetTile::setBoundary(int boundary, int set0, int set1) {
	m_boundary = boundary;
	m_tileCaches[0] = mTileCacheSetGetPointer(&m_cacheSet->tiles, set0);
	m_tileCaches[1] = mTileCacheSetGetPointer(&m_cacheSet->tiles, set1);
}

void AssetTile::selectIndex(int index) {
	m_index = index;
	const color_t* data;
	mTileCache* tileCache = m_tileCaches[index >= m_boundary];

	unsigned bpp = 8 << tileCache->bpp;
	int paletteId = m_paletteId;
	int base = m_addressBase;
	if (index >= m_boundary) {
		base = m_boundaryBase;
		index -= m_boundary;
	}
	int dispIndex = index;
	if (m_addressWidth == 4 && index >= m_boundary / 2) {
		dispIndex -= m_boundary / 2;
	}
	data = mTileCacheGetTile(tileCache, index, paletteId);
	m_ui.tileId->setText(QString::number(dispIndex));
	m_ui.paletteId->setText(QString::number(paletteId));
	m_ui.address->setText(tr("%0%1%2")
		.arg(m_addressWidth == 4 ? index >= m_boundary / 2 : 0)
		.arg(m_addressWidth == 4 ? ":" : "x")
		.arg(dispIndex * bpp | base, m_addressWidth, 16, QChar('0')));
	int flip = 0;
	if (m_flipH) {
		flip |= 007;
	}
	if (m_flipV) {
		flip |= 070;
	}
	for (int i = 0; i < 64; ++i) {
		m_ui.preview->setColor(i ^ flip, data[i]);
	}
	m_ui.preview->update();
}

void AssetTile::setFlip(bool h, bool v) {
	m_flipH = h;
	m_flipV = v;
	selectIndex(m_index);
}

void AssetTile::selectColor(int index) {
	const color_t* data;
	mTileCache* tileCache = m_tileCaches[m_index >= m_boundary];
	unsigned bpp = 8 << tileCache->bpp;
	int paletteId = m_paletteId;
	data = mTileCacheGetTile(tileCache, m_index, m_paletteId);
	color_t color = data[index];
	m_ui.color->setColor(0, color);
	m_ui.color->update();

	uint32_t r = color & 0xF8;
	uint32_t g = (color >> 8) & 0xF8;
	uint32_t b = (color >> 16) & 0xF8;
	m_ui.r->setText(tr("0x%0 (%1)").arg(r, 2, 16, QChar('0')).arg(r, 2, 10, QChar('0')));
	m_ui.g->setText(tr("0x%0 (%1)").arg(g, 2, 16, QChar('0')).arg(g, 2, 10, QChar('0')));
	m_ui.b->setText(tr("0x%0 (%1)").arg(b, 2, 16, QChar('0')).arg(b, 2, 10, QChar('0')));
}

void AssetTile::setCustomProperty(const QString& id, const QVariant& value) {
	QLabel* label = m_customProperties[id];
	if (!label) {
		return;
	}
	label->setText(value.toString());
}
