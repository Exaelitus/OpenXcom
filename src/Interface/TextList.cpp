/*
 * Copyright 2010 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "TextList.h"
#include <cstdarg>
#include <cmath>
#include "../Engine/Action.h"
#include "../Engine/Font.h"
#include "../Engine/Palette.h"
#include "ArrowButton.h"

namespace OpenXcom
{

/**
 * Sets up a blank list with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
TextList::TextList(int width, int height, int x, int y) : InteractiveSurface(width, height, x, y), _texts(), _columns(), _big(0), _small(0), _scroll(0), _visibleRows(0), _color(0), _align(ALIGN_LEFT), _dot(false), _selectable(false), _selRow(0), _bg(0), _selector(0), _margin(0),
																									_arrowLeft(), _arrowRight(), _arrowPos(-1), _leftClick(0), _leftPress(0), _leftRelease(0), _rightClick(0), _rightPress(0), _rightRelease(0)
{
	_up = new ArrowButton(ARROW_BIG_UP, 13, 14, getX() + getWidth() + 4, getY() + 1);
	_up->setVisible(false);
	_up->setTextList(this);
	_down = new ArrowButton(ARROW_BIG_DOWN, 13, 14, getX() + getWidth() + 4, getY() + getHeight() - 12);
	_down->setVisible(false);
	_down->setTextList(this);
}

/**
 * Deletes all the stuff contained in the list.
 */
TextList::~TextList()
{
	for (std::vector< std::vector<Text*> >::iterator u = _texts.begin(); u < _texts.end(); u++)
	{
		for (std::vector<Text*>::iterator v = (*u).begin(); v < (*u).end(); v++)
		{
			delete *v;
		}
	}
	for (std::vector<ArrowButton*>::iterator i = _arrowLeft.begin(); i < _arrowLeft.end(); i++)
	{
		delete *i;
	}
	for (std::vector<ArrowButton*>::iterator i = _arrowRight.begin(); i < _arrowRight.end(); i++)
	{
		delete *i;
	}
	delete _selector;
	delete _up;
	delete _down;
}

/**
 * Unpresses all the arrow buttons.
 * @param state Pointer to running state.
 */
void TextList::unpress(State *state)
{
	InteractiveSurface::unpress(state);
	for (std::vector<ArrowButton*>::iterator i = _arrowLeft.begin(); i < _arrowLeft.end(); i++)
	{
		(*i)->unpress(state);
	}
	for (std::vector<ArrowButton*>::iterator i = _arrowRight.begin(); i < _arrowRight.end(); i++)
	{
		(*i)->unpress(state);
	}
}

/**
 * Returns a pointer to a certain Text object in the list.
 * Useful for customizing or getting values off an individual cell.
 * @note If a Text is changed, a redraw needs to be manually requested.
 * @param row Row number.
 * @param col Column number.
 * @return Pointer to the requested Text.
 */
Text *const TextList::getCell(int row, int col) const
{
	return _texts[row][col];
}

/**
 * Adds a new row of text to the list, automatically creating
 * the required Text objects lined up where they need to be.
 * @param cols Number of columns.
 * @param ... Text for each cell in the new row.
 */
void TextList::addRow(int cols, ...)
{
	va_list args;
	va_start(args, cols);
	std::vector<Text*> temp;
	int rowX = 0;

	for (int i = 0; i < cols; i++)
	{
		Text* txt = new Text(_columns[i], _small->getHeight(), _margin + rowX, getY());
		txt->setPalette(this->getPalette());
		txt->setFonts(_big, _small);
		
		std::wstring buf = va_arg(args, wchar_t*);
		// Places dots between text
		if (_dot && i < cols - 1)
		{
			int w = 0;
			for (std::wstring::iterator c = buf.begin(); c < buf.end(); c++)
			{
				if (*c == ' ')
				{
					w += _small->getWidth() / 2;
				}
				else
				{
					w += _small->getChar(*c)->getCrop()->w + _small->getSpacing();
				}
			}
			while (w < _columns[i])
			{
				w += _small->getChar('.')->getCrop()->w + _small->getSpacing();
				buf += '.';
			}
		}
		txt->setText(buf);

		txt->setColor(_color);
		txt->setAlign(_align);
		txt->setSmall();
		temp.push_back(txt);
		rowX += _columns[i];
	}
	_texts.push_back(temp);

	if (_arrowPos != -1)
	{
		ArrowButton *a1 = new ArrowButton(ARROW_SMALL_UP, 11, 8, getX() + _arrowPos, getY());
		a1->setPalette(this->getPalette());
		a1->setColor(_up->getColor());
		a1->onMouseClick(_leftClick);
		a1->onMousePress(_leftPress);
		a1->onMouseRelease(_leftRelease);
		_arrowLeft.push_back(a1);
		ArrowButton *a2 = new ArrowButton(ARROW_SMALL_DOWN, 11, 8, getX() + _arrowPos + 12, getY());
		a2->setPalette(this->getPalette());
		a2->setColor(_up->getColor());
		a2->onMouseClick(_rightClick);
		a2->onMousePress(_rightPress);
		a2->onMouseRelease(_rightRelease);
		_arrowRight.push_back(a2);
	}

	draw();
	va_end(args);
	updateArrows();
}

/**
 * Changes the columns that the list contains.
 * While rows can be unlimited, columns need to be specified
 * since they can have various widths for lining up the text.
 * @param cols Number of columns.
 * @param ... Width of each column.
 */
void TextList::setColumns(int cols, ...)
{
	va_list args;
	va_start(args, cols);

	for (int i = 0; i < cols; i++)
	{
		_columns.push_back(va_arg(args, int));
	}

	va_end(args);
}

/**
 * Replaces a certain amount of colors in the palette of all
 * the text contained in the list.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void TextList::setPalette(SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	for (std::vector< std::vector<Text*> >::iterator u = _texts.begin(); u < _texts.end(); u++)
	{
		for (std::vector<Text*>::iterator v = u->begin(); v < u->end(); v++)
		{
			(*v)->setPalette(colors, firstcolor, ncolors);
		}
	}
	for (std::vector<ArrowButton*>::iterator i = _arrowLeft.begin(); i < _arrowLeft.end(); i++)
	{
		(*i)->setPalette(colors, firstcolor, ncolors);
	}
	for (std::vector<ArrowButton*>::iterator i = _arrowRight.begin(); i < _arrowRight.end(); i++)
	{
		(*i)->setPalette(colors, firstcolor, ncolors);
	}
	if (_selector != 0)
	{
		_selector->setPalette(colors, firstcolor, ncolors);
	}
	_up->setPalette(colors, firstcolor, ncolors);
	_down->setPalette(colors, firstcolor, ncolors);
}

/**
 * Changes the various fonts of the text in the list
 * and calculates the selector and visible amount of rows.
 * @param big Pointer to large-size font.
 * @param small Pointer to small-size font.
 */
void TextList::setFonts(Font *big, Font *small)
{
	_big = big;
	_small = small;
	
	delete _selector;
	_selector = new Surface(getWidth(), _small->getHeight() + _small->getSpacing(), getX(), getY());
	_selector->setPalette(getPalette());
	_selector->setVisible(false);

	for (int y = 0; y < getHeight(); y += _small->getHeight() + _small->getSpacing())
	{
		_visibleRows++;
	}
}

/**
 * Changes the color of the text in the list. This doesn't change
 * the color of existing text, just the color of text added from then on.
 * @param color Color value.
 */
void TextList::setColor(Uint8 color)
{
	_color = color;
}

/**
 * Returns the color of the text in the list.
 * @return Color value.
 */
Uint8 TextList::getColor() const
{
	return _color;
}

/**
 * Changes the horizontal alignment of the text in the list. This doesn't change
 * the alignment of existing text, just the alignment of text added from then on.
 * @param align Horizontal alignment.
 */
void TextList::setAlign(TextHAlign align)
{
	_align = align;
}

/**
 * If enabled, the text in different columns will be separated by dots.
 * Otherwise, it will only be separated by blank space.
 * @param dot True for dots, False for spaces.
 */
void TextList::setDot(bool dot)
{
	_dot = dot;
}

/**
 * If enabled, the list will respond to player input,
 * highlighting selected rows and receiving clicks.
 * @param selectable Selectable setting.
 */
void TextList::setSelectable(bool selectable)
{
	_selectable = selectable;
}

/**
 * Returns the currently selected row if the text
 * list is selectable.
 * @return Selected row.
 */
int TextList::getSelectedRow() const
{
	return _selRow;
}

/**
 * Changes the surface used to draw the background of the selector.
 * @param bg New background.
 */
void TextList::setBackground(Surface *bg)
{
	_bg = bg;
}

/**
 * Changes the horizontal margin placed around the text.
 * @param margin Margin in pixels.
 */
void TextList::setMargin(int margin)
{
	_margin = margin;
}

/**
 * Changes the color of the arrow buttons in the list.
 * @param color Color value.
 */
void TextList::setArrowColor(Uint8 color)
{
	_up->setColor(color);
	_down->setColor(color);
}

/**
 * Sets the position of the column of arrow buttons
 * in the text list.
 * @param pos X in pixels (-1 to disable).
 */
void TextList::setArrowColumn(int pos)
{
	_arrowPos = pos;
}

/**
 * Sets a function to be called everytime the left arrows are mouse clicked.
 * @param handler Action handler.
 */
void TextList::onLeftArrowClick(ActionHandler handler)
{
	_leftClick = handler;
	for (std::vector<ArrowButton*>::iterator i = _arrowLeft.begin(); i < _arrowLeft.end(); i++)
	{
		(*i)->onMouseClick(handler);
	}
}

/**
 * Sets a function to be called everytime the left arrows are mouse pressed.
 * @param handler Action handler.
 */
void TextList::onLeftArrowPress(ActionHandler handler)
{
	_leftPress = handler;
	for (std::vector<ArrowButton*>::iterator i = _arrowLeft.begin(); i < _arrowLeft.end(); i++)
	{
		(*i)->onMousePress(handler);
	}
}

/**
 * Sets a function to be called everytime the left arrows are mouse released.
 * @param handler Action handler.
 */
void TextList::onLeftArrowRelease(ActionHandler handler)
{
	_leftRelease = handler;
	for (std::vector<ArrowButton*>::iterator i = _arrowLeft.begin(); i < _arrowLeft.end(); i++)
	{
		(*i)->onMouseRelease(handler);
	}
}

/**
 * Sets a function to be called everytime the right arrows are mouse clicked.
 * @param handler Action handler.
 */
void TextList::onRightArrowClick(ActionHandler handler)
{
	_rightClick = handler;
	for (std::vector<ArrowButton*>::iterator i = _arrowRight.begin(); i < _arrowRight.end(); i++)
	{
		(*i)->onMouseClick(handler);
	}
}

/**
 * Sets a function to be called everytime the right arrows are mouse pressed.
 * @param handler Action handler.
 */
void TextList::onRightArrowPress(ActionHandler handler)
{
	_rightPress = handler;
	for (std::vector<ArrowButton*>::iterator i = _arrowRight.begin(); i < _arrowRight.end(); i++)
	{
		(*i)->onMousePress(handler);
	}
}

/**
 * Sets a function to be called everytime the right arrows are mouse released.
 * @param handler Action handler.
 */
void TextList::onRightArrowRelease(ActionHandler handler)
{
	_rightRelease = handler;
	for (std::vector<ArrowButton*>::iterator i = _arrowRight.begin(); i < _arrowRight.end(); i++)
	{
		(*i)->onMouseRelease(handler);
	}
}

/**
 * Removes all the rows currently stored in the list.
 */
void TextList::clearList()
{
	for (std::vector< std::vector<Text*> >::iterator u = _texts.begin(); u < _texts.end(); u++)
	{
		for (std::vector<Text*>::iterator v = u->begin(); v < u->end(); v++)
		{
			delete (*v);
		}
		u->clear();
	}
	_texts.clear();
}

/**
 * Scrolls the text in the list up by one row.
 */
void TextList::scrollUp()
{
	if (_texts.size() > _visibleRows && _scroll > 0)
	{
		_scroll--;
		draw();
	}
	updateArrows();
}

/**
 * Scrolls the text in the list down by one row.
 */
void TextList::scrollDown()
{
	if (_texts.size() > _visibleRows && _scroll < _texts.size() - _visibleRows)
	{
		_scroll++;
		draw();
	}
	updateArrows();
}

/**
 * Updates the visibility of the arrow buttons according to
 * the current scroll position.
 */
void TextList::updateArrows()
{
	_up->setVisible((_texts.size() > _visibleRows && _scroll > 0));
	_down->setVisible((_texts.size() > _visibleRows && _scroll < _texts.size() - _visibleRows));
}

/**
 * Draws the text list and all the text contained within.
 */
void TextList::draw()
{
	clear();
	for (unsigned int i = _scroll; i < _texts.size() && i < _scroll + _visibleRows; i++)
	{
		for (std::vector<Text*>::iterator j = _texts[i].begin(); j < _texts[i].end(); j++)
		{
			(*j)->setY((i - _scroll) * (_small->getHeight() + _small->getSpacing()));
            (*j)->blit(this);
        }
    }
}

/**
 * Blits the text list and selector.
 * @param surface Pointer to surface to blit onto.
 */
void TextList::blit(Surface *surface)
{
	if (_visible && !_hidden)
	{
		_selector->blit(surface);
	}
	Surface::blit(surface);
	if (_visible && !_hidden)
	{
		_up->blit(surface);
		_down->blit(surface);
		if (_arrowPos != -1)
		{
			for (unsigned int i = _scroll; i < _texts.size() && i < _scroll + _visibleRows; i++)
			{
				_arrowLeft[i]->setY(getY() + (i - _scroll) * (_small->getHeight() + _small->getSpacing()));
				_arrowLeft[i]->blit(surface);
				_arrowRight[i]->setY(getY() + (i - _scroll) * (_small->getHeight() + _small->getSpacing()));
				_arrowRight[i]->blit(surface);
			}
		}
	}
}

/**
 * Passes events to arrow buttons.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void TextList::handle(Action *action, State *state)
{
	InteractiveSurface::handle(action, state);
	_up->handle(action, state);
	_down->handle(action, state);
	if (_arrowPos != -1)
	{
		for (unsigned int i = _scroll; i < _texts.size() && i < _scroll + _visibleRows; i++)
		{
			_arrowLeft[i]->handle(action, state);
			_arrowRight[i]->handle(action, state);
		}
	}
}

/**
 * Passes ticks to arrow buttons.
 */
void TextList::think()
{
	InteractiveSurface::think();
	_up->think();
	_down->think();
	for (std::vector<ArrowButton*>::iterator i = _arrowLeft.begin(); i < _arrowLeft.end(); i++)
	{
		(*i)->think();
	}
	for (std::vector<ArrowButton*>::iterator i = _arrowRight.begin(); i < _arrowRight.end(); i++)
	{
		(*i)->think();
	}
}

/**
 * Ignores any mouse clicks that aren't on a row with the left mouse button.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void TextList::mousePress(Action *action, State *state)
{
	if (_selectable && action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_selRow < _texts.size())
		{
			InteractiveSurface::mousePress(action, state);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
	{
		scrollUp();
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
	{
		scrollDown();
	}
}

/*
 * Ignores any mouse clicks that aren't on a row with the left mouse button.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void TextList::mouseRelease(Action *action, State *state)
{
	if (_selectable && action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_selRow < _texts.size())
		{
			InteractiveSurface::mouseRelease(action, state);
		}
	}
}

/**
 * Ignores any mouse clicks that aren't on a row with the left mouse button.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void TextList::mouseClick(Action *action, State *state)
{
	if (_selectable && action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_selRow < _texts.size())
		{
			InteractiveSurface::mouseClick(action, state);
		}
	}
}

/**
 * Selects the row the mouse is over.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void TextList::mouseOver(Action *action, State *state)
{
	if (_selectable)
	{
		int h = _small->getHeight() + _small->getSpacing();
		double y = action->getYMouse() - getY() * action->getYScale();
		_selRow = _scroll + (int)floor(y / (h * action->getYScale()));

		if (_selRow < _texts.size())
		{
			_selector->setY(getY() + (_selRow - _scroll) * h);
			_selector->copy(_bg);
			_selector->offset(-10, Palette::backPos);
			_selector->setVisible(true);
		}
		else
		{
			_selector->setVisible(false);
		}
	}

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Deselects the row.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void TextList::mouseOut(Action *action, State *state)
{
	if (_selectable)
	{
		_selector->setVisible(false);
	}

	InteractiveSurface::mouseOut(action, state);
}

}
