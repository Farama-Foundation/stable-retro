/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#pragma once

#include "Display.h"

#include <QImage>
#include <QTimer>

namespace QGBA {

class DisplayQt : public Display {
Q_OBJECT

public:
	DisplayQt(QWidget* parent = nullptr);

	void startDrawing(std::shared_ptr<CoreController>) override;
	bool isDrawing() const override { return m_isDrawing; }
	bool supportsShaders() const override { return false; }
	VideoShader* shaders() override { return nullptr; }

public slots:
	void stopDrawing() override;
	void pauseDrawing() override { m_isDrawing = false; }
	void unpauseDrawing() override { m_isDrawing = true; }
	void forceDraw() override { update(); }
	void lockAspectRatio(bool lock) override;
	void lockIntegerScaling(bool lock) override;
	void filter(bool filter) override;
	void framePosted() override;
	void setShaders(struct VDir*) override {}
	void clearShaders() override {}

protected:
	virtual void paintEvent(QPaintEvent*) override;

private:
	bool m_isDrawing = false;
	unsigned m_width;
	unsigned m_height;
	QImage m_backing{nullptr};
	std::shared_ptr<CoreController> m_context = nullptr;
};

}
