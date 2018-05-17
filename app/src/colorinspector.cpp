/*

Pencil - Traditional Animation Software
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "colorinspector.h"
#include "ui_colorinspector.h"

#include <QSettings>
#include <QStyleOption>
#include <QDebug>
#include <QStylePainter>

#include "colorslider.h"
#include "pencildef.h"

ColorInspector::ColorInspector(QWidget *parent) :
    BaseDockWidget(parent)
{

    QWidget* innerWidget = new QWidget;
    setWindowTitle(tr("Color Inspector", "Window title of color inspector"));

    ui = new Ui::ColorInspector;
    ui->setupUi(innerWidget);
    setWidget(innerWidget);

    QButtonGroup* colorModeChangeGroup = new QButtonGroup();

    colorModeChangeGroup->addButton(ui->hsvButton);
    colorModeChangeGroup->addButton(ui->rgbButton);
    colorModeChangeGroup->setExclusive(true);

    QSettings settings(PENCIL2D, PENCIL2D);
    isRgbColors = settings.value("isRgb").toBool();

    if (isRgbColors) {
        ui->rgbButton->setChecked(true);
    } else {
        ui->hsvButton->setChecked(true);
    }
    onModeChanged();

    auto spinBoxChanged = static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    connect(ui->RedspinBox, spinBoxChanged, this, &ColorInspector::onColorChanged);
    connect(ui->GreenspinBox, spinBoxChanged, this, &ColorInspector::onColorChanged);
    connect(ui->BluespinBox, spinBoxChanged, this, &ColorInspector::onColorChanged);
    connect(ui->AlphaspinBox, spinBoxChanged, this, &ColorInspector::onColorChanged);
    connect(ui->rgbButton, &QPushButton::clicked, this, &ColorInspector::onModeChanged);
    connect(ui->hsvButton, &QPushButton::clicked, this, &ColorInspector::onModeChanged);

    connect(ui->red_slider, &ColorSlider::valueChanged, this, &ColorInspector::onSliderChanged);
    connect(ui->green_slider, &ColorSlider::valueChanged, this, &ColorInspector::onSliderChanged);
    connect(ui->blue_slider, &ColorSlider::valueChanged, this, &ColorInspector::onSliderChanged);
    connect(ui->alpha_slider, &ColorSlider::valueChanged, this, &ColorInspector::onSliderChanged);
}

ColorInspector::~ColorInspector()
{
    delete ui;
}

void ColorInspector::initUI()
{
    if (isRgbColors) {
        ui->red_slider->init(ColorSlider::ColorType::RED, QColor(255,255,255,255), 0.0, 255.0);
        ui->green_slider->init(ColorSlider::ColorType::GREEN, QColor(255,255,255,255), 0.0, 255.0);
        ui->blue_slider->init(ColorSlider::ColorType::BLUE, QColor(255,255,255,255), 0.0, 255.0);
        ui->alpha_slider->init(ColorSlider::ColorType::ALPHA, QColor(0,255,255,255), 0.0, 255.0);
    } else {
        ui->red_slider->init(ColorSlider::ColorType::HUE, QColor(359,255,255,255), 0.0, 359.0);
        ui->green_slider->init(ColorSlider::ColorType::SAT, QColor(255,255,255,255), 0.0, 255.0);
        ui->blue_slider->init(ColorSlider::ColorType::VAL, QColor(255,255,255,255), 0.0, 255.0);
        ui->alpha_slider->init(ColorSlider::ColorType::ALPHA, QColor(0,255,255,255), 0.0, 255.0);
    }

    QSettings settings(PENCIL2D, PENCIL2D);

    QColor savedColor;
    savedColor.setRgba(settings.value("colorOfSliders").toUInt());
    setColor(savedColor);
}

void ColorInspector::updateUI()
{
}

void ColorInspector::onSliderChanged(QColor color)
{

    if (isRgbColors) {
        ui->red_slider->setRgb(color);
        ui->green_slider->setRgb(color);
        ui->blue_slider->setRgb(color);
        ui->alpha_slider->setRgb(color);
    } else {
        ui->red_slider->setHsv(color);
        ui->green_slider->setHsv(color);
        ui->blue_slider->setHsv(color);
        ui->alpha_slider->setHsv(color);
    }

    emit colorChanged(color);
}

void ColorInspector::setColor(QColor newColor)
{
    // compare under the same color spec
    newColor = (isRgbColors) ? newColor.toRgb() : newColor.toHsv();

    if (newColor == mCurrentColor)
    {
        return;
    }
    noColorUpdate = true;

    qDebug() << "set color";
    if(isRgbColors)
    {
        ui->red_slider->setRgb(newColor);
        ui->green_slider->setRgb(newColor);
        ui->blue_slider->setRgb(newColor);
        ui->alpha_slider->setRgb(newColor);

        ui->RedspinBox->setValue(newColor.red());
        ui->GreenspinBox->setValue(newColor.green());
        ui->BluespinBox->setValue(newColor.blue());
        ui->AlphaspinBox->setValue(newColor.alpha());
    }
    else
    {
        ui->red_slider->setHsv(newColor);
        ui->green_slider->setHsv(newColor);
        ui->blue_slider->setHsv(newColor);
        ui->alpha_slider->setHsv(newColor);

        ui->RedspinBox->setValue(newColor.hsvHue());
        ui->GreenspinBox->setValue(qRound(newColor.hsvSaturation() / 2.55));
        ui->BluespinBox->setValue(qRound(newColor.value() / 2.55));
        ui->AlphaspinBox->setValue(qRound(newColor.alpha() / 2.55));
    }

    mCurrentColor = newColor;

    QPalette p1 = ui->colorWrapper->palette(), p2 = ui->color->palette();
    p1.setBrush(QPalette::Background, QBrush(QImage(":/background/checkerboard.png")));
    p2.setColor(QPalette::Background, mCurrentColor);
    ui->colorWrapper->setPalette(p1);
    ui->color->setPalette(p2);
    noColorUpdate = false;
    update();
}

QColor ColorInspector::color()
{
    return mCurrentColor;
}

void ColorInspector::paintEvent(QPaintEvent*)
{

    // HACK: possible bug in 5.9
    // title style is not set when window is not docked
    // this enforces the style again. This is what QDockWidget
    // should be doing behind the scene
    QStyleOptionDockWidget opt;
    initStyleOption(&opt);

    QStylePainter p(this);
    p.drawControl(QStyle::CE_DockWidgetTitle, opt);
}

void ColorInspector::mouseReleaseEvent(QMouseEvent*)
{

    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("colorOfSliders", mCurrentColor.rgba());

}

void ColorInspector::onModeChanged()
{
    // assume hsv if not checked
    bool newValue = ui->rgbButton->isChecked();

    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("isRgb", newValue);

    isRgbColors = newValue;
    noColorUpdate = true;

    if (isRgbColors)
    {
        ui->red->setText("R");
        ui->green->setText("G");
        ui->blue->setText("B");
        ui->alpha->setText("A");

        ui->RedspinBox->setRange(0,255);
        ui->RedspinBox->setSuffix("");
        ui->GreenspinBox->setRange(0,255);
        ui->GreenspinBox->setSuffix("");
        ui->BluespinBox->setRange(0,255);
        ui->BluespinBox->setSuffix("");
        ui->AlphaspinBox->setRange(0,255);
        ui->AlphaspinBox->setSuffix("");
        mCurrentColor = mCurrentColor.toRgb();

        ui->red_slider->setMax(255);
        ui->red_slider->setColorType(ColorSlider::ColorType::RED);
        ui->red_slider->setColorSpecType(ColorSlider::ColorSpecType::RGB);
        ui->green_slider->setColorSpecType(ColorSlider::ColorSpecType::RGB);
        ui->green_slider->setColorType(ColorSlider::ColorType::GREEN);
        ui->blue_slider->setColorSpecType(ColorSlider::ColorSpecType::RGB);
        ui->blue_slider->setColorType(ColorSlider::ColorType::BLUE);
        ui->alpha_slider->setColorSpecType(ColorSlider::ColorSpecType::RGB);
        ui->alpha_slider->setColorType(ColorSlider::ColorType::ALPHA);

        ui->RedspinBox->setValue(mCurrentColor.red());
        ui->GreenspinBox->setValue(mCurrentColor.green());
        ui->BluespinBox->setValue(mCurrentColor.blue());
        ui->AlphaspinBox->setValue(mCurrentColor.alpha());
    }
    else
    {
        ui->red->setText("H");
        ui->green->setText("S");
        ui->blue->setText("V");
        ui->alpha->setText("A");

        ui->red_slider->setMax(359);
        ui->red_slider->setColorType(ColorSlider::ColorType::HUE);
        ui->red_slider->setColorSpecType(ColorSlider::ColorSpecType::HSV);
        ui->green_slider->setColorType(ColorSlider::ColorType::SAT);
        ui->green_slider->setColorSpecType(ColorSlider::ColorSpecType::HSV);
        ui->blue_slider->setColorType(ColorSlider::ColorType::VAL);
        ui->blue_slider->setColorSpecType(ColorSlider::ColorSpecType::HSV);
        ui->alpha_slider->setColorType(ColorSlider::ColorType::ALPHA);
        ui->alpha_slider->setColorSpecType(ColorSlider::ColorSpecType::HSV);

        ui->RedspinBox->setRange(0,359);
        ui->RedspinBox->setSuffix("°");
        ui->GreenspinBox->setRange(0,100);
        ui->GreenspinBox->setSuffix("%");
        ui->BluespinBox->setRange(0,100);
        ui->BluespinBox->setSuffix("%");
        ui->AlphaspinBox->setRange(0,100);
        ui->AlphaspinBox->setSuffix("%");

        qreal bound = 100.0/255.0; // from 255 to 100
        mCurrentColor = mCurrentColor.toHsv();
        ui->RedspinBox->setValue(mCurrentColor.hsvHue());
        ui->GreenspinBox->setValue(qRound(mCurrentColor.hsvSaturation()*bound));
        ui->BluespinBox->setValue(qRound(mCurrentColor.value()*bound));
        ui->AlphaspinBox->setValue(qRound(mCurrentColor.alpha()*bound));
    }

    noColorUpdate = false;
    emit modeChange(isRgbColors);
}

void ColorInspector::onColorChanged()
{
    if(noColorUpdate) return;

    QColor c;
    if (isRgbColors) {
        c.setRgb(
            ui->RedspinBox->value(),
            ui->GreenspinBox->value(),
            ui->BluespinBox->value(),
            ui->AlphaspinBox->value());

    } else {
        c.setHsv(
            ui->RedspinBox->value(),
            ui->GreenspinBox->value()* 2.55,
            ui->BluespinBox->value()* 2.55,
            ui->AlphaspinBox->value()* 2.55);
    }

    emit colorChanged(c);
}
