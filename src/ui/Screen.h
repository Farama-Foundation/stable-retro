#pragma once

#include <QOpenGLFunctions_1_1>
#include <QOpenGLTexture>
#include <QOpenGLWidget>

#include <memory>

class Screen : public QOpenGLWidget, QOpenGLFunctions_1_1 {
	Q_OBJECT
public:
	Screen(QWidget* parent);
	~Screen();

public slots:
	void setImage(const QImage&, int rotation);
	void setPaused(bool);
	void gamesWereBetter() { m_nostalgia ^= 1; }

signals:
	void pauseChanged(bool);

protected:
	void initializeGL() override;
	void paintGL() override;
	bool event(QEvent*) override;

	std::unique_ptr<QOpenGLTexture> m_texture;
	int m_rotation = 0;
	bool m_nostalgia = false;
	bool m_started = false;
	bool m_paused = false;
};
