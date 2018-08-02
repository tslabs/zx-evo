#include "std.h"
#include "emul.h"
#include "vars.h"
#include "dbgpaint.h"
#include "dbgwidgets.h"
#include <algorithm>
#include <memory>

dbg_control::dbg_control(int h):h_(h) { }

void dbg_control::check_parent() const
{
	if (column_ == nullptr || canvas_ == nullptr)
		throw exception("parent not assigned");
}

void dbg_control::draw_reg_frame(const char *title, const u8 *value) const
{
	check_parent();
	canvas_->draw_reg_frame(*this, get_h(), title, value);
}

void dbg_control::draw_bits_range(int bits) const
{
	check_parent();

	char line[3] = "  ";

	if (bits >= 0)
	{
		if (bits >= 10)
			line[0] = static_cast<char>(48 + bits / 10);

		if (bits % 10 == 0)
			line[1] = '0';
		else
			line[1] = static_cast<char>(48 + bits % 10);

		(*canvas_)
			.draw_text(line, W_BITS)
			.move_x_to_len()
			.draw_text(":", W_BITS);
	}
	else
		(*canvas_).draw_text("   ", W_BITS);

	(*canvas_).next_col();
}

void dbg_control::draw_bit(char* title, int bits, const char* val_set[], int val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, W_NORM)
		.next_col()
		.draw_text("=", W_EQ)
		.next_col();

	(*canvas_)
		.draw_text(val_set[val], W_NORM);

	next_row();
}

void dbg_control::draw_bit(char* title, int bits, u8 val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, W_NORM)
		.next_col()
		.draw_text("=", W_EQ)
		.next_col();

  (*canvas_)
    .draw_text((val & 1) ? "ena" : "dis", W_NORM);
  next_row();
}

void dbg_control::draw_hex8_inline(char* title, u8 val) const
{
	check_parent();
	(*canvas_)
		.draw_text(title, W_NORM).move_x_to_len()
		.draw_text("= ", W_EQ).move_x_to_len()
		.draw_hex(val, 2, W_NORM).move_x_to_len()
		.draw_text(" ", W_NORM).move_x_to_len();
}

void dbg_control::draw_dec_inline(char* title, u16 val) const
{
	check_parent();
	(*canvas_)
		.draw_text(title, W_NORM).move_x_to_len()
		.draw_text("= ", W_EQ).move_x_to_len()
		.draw_number(val, W_NORM).move_x_to_len()
		.draw_text(" ", W_NORM).move_x_to_len();
}

void dbg_control::draw_hex16(char* title, int bits, u16 val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, W_NORM)
		.next_col()
		.draw_text("=", W_EQ)
		.next_col();

	(*canvas_)
		.draw_hex(val, 4, W_NORM);

	next_row();
}

void dbg_control::draw_hex24(char* title, int bits, u32 val) const
{
	check_parent();

	(*canvas_)
		.draw_text(title, W_NORM)
		.next_col()
		.next_col()
		.draw_text("=", W_EQ)
		.next_col();

	(*canvas_)
		.draw_hex(val >> 14, 2, W_NORM).move_x_to_len()
		.draw_text(":", W_EQ).move_x_to_len()
		.draw_hex(val & 0x3FFF, 4, W_NORM);

	next_row();
}

void dbg_control::draw_bit_d(char* title, int bits, u8 val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, W_NORM)
		.next_col()
		.draw_text("=", W_EQ)
		.next_col();

	(*canvas_)
		.draw_number(val, W_NORM);

	next_row();
}

void dbg_control::draw_bit_h(char* title, int bits, u8 val) const
{
    check_parent();
    draw_bits_range(bits);

    (*canvas_)
        .draw_text(title, W_NORM)
        .next_col()
        .draw_text("=", W_EQ)
        .next_col();

    (*canvas_)
        .draw_hex(val, 2, W_NORM);

    next_row();
}

void dbg_control::draw_port(char* title, u8 val) const
{
	check_parent();

	(*canvas_)
		.draw_text(title, W_NORM)
		.next_col()
		.next_col()
		.draw_text("=", W_EQ)
		.next_col();

	(*canvas_)
		.draw_hex(val, 2, W_NORM);

	next_row();
}

void dbg_control::draw_hl_port(char prefix, u8 hval, u8 lval, u16 val) const
{
	check_parent();

	char line[3] = "  ";
	char line1[2] = " ";
	line[0] = prefix;
	line1[0] = prefix;

	line[1] = 'H';
	(*canvas_)
		.draw_text(line, W_NORM).move_x_to_len()
		.draw_text("= ", W_EQ).move_x_to_len()
		.draw_hex(hval, 2, W_NORM).move_x_to_len()
		.draw_text(" ").move_x_to_len();

	line[1] = 'L';
	(*canvas_)
		.draw_text(line, W_NORM).move_x_to_len()
		.draw_text("= ", W_EQ).move_x_to_len()
		.draw_hex(lval, 2, W_NORM).move_x_to_len()
		.draw_text(" ").move_x_to_len();

	(*canvas_)
		.draw_text(line1, W_NORM).move_x_to_len()
		.draw_text("= ", W_EQ).move_x_to_len()
		.draw_hex(val, 4, W_NORM).move_x_to_len()
		.draw_text(" ").move_x_to_len();

	next_row();
}

void dbg_control::draw_xhl_port(char prefix, u8 xval, u8 hval, u8 lval) const
{
	check_parent();

	char line[3] = "  ";
	line[0] = prefix;

	line[1] = 'X';
	(*canvas_)
		.draw_text(line, W_NORM).move_x_to_len()
		.draw_text("= ", W_EQ).move_x_to_len()
		.draw_hex(xval, 2, W_NORM).move_x_to_len()
		.draw_text(" ").move_x_to_len();

	line[1] = 'H';
	(*canvas_)
		.draw_text(line, W_NORM).move_x_to_len()
		.draw_text("= ", W_EQ).move_x_to_len()
		.draw_hex(hval, 2, W_NORM).move_x_to_len()
		.draw_text(" ").move_x_to_len();

	line[1] = 'L';
	(*canvas_)
		.draw_text(line, W_NORM).move_x_to_len()
		.draw_text("= ", W_EQ).move_x_to_len()
		.draw_hex(lval, 2, W_NORM).move_x_to_len()
		.draw_text(" ").move_x_to_len();

	next_row();
}

void dbg_control::draw_led(char* title, u8 on) const
{
	check_parent();

	(*canvas_).move(1, 0);

	if (on & 1)
		(*canvas_).draw_text(title, W_LEDON).move_x_to_len();
	else
		(*canvas_).move(strlen(title), 0);
}

void dbg_control::set_xy(int x, int y) const
{
	canvas_->set_xy(column_->get_x() + x, get_y() + y);
}

void dbg_control::next_row() const
{
	canvas_->next_row();
}

void dbg_control::draw_text_on_line(const char* text, int x, u8 attr) const
{
	canvas_->set_x(column_->get_x() + x);
	canvas_->draw_text(text, attr);
}

int dbg_control::get_h() const
{
	return h_;
}

void dbg_control::set_y(int y)
{
	y_ = y;
}

const dbg_column& dbg_control::get_column() const
{
	return *column_;
}

void dbg_control::set_parent(dbg_column *column, dbg_canvas *canvas, int y)
{
	if (column_ != nullptr || canvas_ != nullptr)
		throw exception("already inited");

	column_ = column;
	canvas_ = canvas;

	y_ = y;
}


void dbg_column::move_y(int dy)
{
	y_ += dy;
}

dbg_column::dbg_column(int w, int x, dbg_canvas& canvas)
: w_(w), x_(x), canvas_(canvas)
{ }

int dbg_column::get_w() const
{
	return w_;
}

int dbg_column::get_x() const
{
	return x_;
}

int dbg_column::get_y() const
{
	return y_;
}


void dbg_column::reset()
{
	y_ = 0;
}

void dbg_column::paint()
{
	for_each(controls_.begin(), controls_.end(), [](dbg_control* item)
	{
		item->on_paint();
	});
}






void dbg_canvas::print_dbg(char* line, int color)
{
	if (color == -1)
		tprint(base_x_ + x_, base_y_ + y_, line, attr_);
	else
		tprint(base_x_ + x_, base_y_ + y_, line, static_cast<u8>(color));

	last_len_ = strlen(line);
}

dbg_canvas::dbg_canvas(int base_x, int base_y)
	: base_x_(base_x),
	base_y_(base_y)
{ }

dbg_canvas::~dbg_canvas()
{
	for_each(columns_.begin(), columns_.end(), default_delete<dbg_column>());
	for_each(controls_.begin(), controls_.end(), default_delete<dbg_control>());
}

dbg_canvas& dbg_canvas::set_attr(const u8 color)
{
	attr_ = color;

	return *this;
}

dbg_canvas& dbg_canvas::set_xy(int x, int y)
{
	x_ = x;
	y_ = y;

	return *this;
}

dbg_canvas& dbg_canvas::set_cols(int col0, int col1, int col2, int col3)
{
	cols_[0] = col0;
	cols_[1] = col1;
	cols_[2] = col2;
	cols_[3] = col3;

	return  *this;
}

dbg_canvas& dbg_canvas::next_row()
{
	curr_col_ = 0;
	const auto x = cols_[curr_col_];

	if (x != -1) {
		set_xy(x, y_ + 1);
	}

	return  *this;
}

dbg_canvas& dbg_canvas::next_col()
{
	curr_col_ = (curr_col_ + 1) & 3;
	const auto x = cols_[curr_col_];

	if (x != -1) {
		set_xy(x, y_);
	}

	return *this;
}

dbg_canvas& dbg_canvas::move_x_to_len()
{
	set_xy(x_ + last_len_, y_);

	return *this;
}

dbg_canvas& dbg_canvas::draw_frame(const int w, int h)
{
	frame(base_x_ + x_, base_y_ + y_, w, h, FRAME);
	return *this;
}

dbg_canvas& dbg_canvas::fill_rect(const int w, const int h)
{
	fillrect(base_x_ + x_, base_y_ + y_, w, h, W_NORM);

	return *this;
}

dbg_canvas& dbg_canvas::draw_text(const char *msg, int color)
{
	char line[0x20];
	sprintf(line, "%s", msg);
	print_dbg(line, color);

	return *this;
}

dbg_canvas& dbg_canvas::draw_number(const int value, int color)
{
	char line[0x20];
	sprintf(line, "%03d", value);
	print_dbg(line, color);

	return *this;
}

dbg_canvas& dbg_canvas::draw_hex(const unsigned value, int len, int color)
{
	char line[0x20];
	char fstr[5] = "%0_X";
	fstr[2] = static_cast<char>(48 + len);

	sprintf(line, fstr, value);
	print_dbg(line, color);

	return *this;
}

dbg_canvas& dbg_canvas::move(int dx, int dy)
{
	x_ += dx;
	y_ += dy;

	return *this;
}

dbg_canvas& dbg_canvas::set_x(int x)
{
	x_ = x;

	return *this;
}

void dbg_canvas::draw_reg_frame(const dbg_control& control, int h, const char* title, const u8 *regval)
{
	const auto x = control.get_column().get_x();
	const auto y = control.get_y();
	const auto w = control.get_column().get_w();

	set_cols(x + 0, x + 3, x + 12, x + 14);
	set_xy(x, y);

	draw_text(title, W_TITLE);
	move_x_to_len();

	if (regval != nullptr)
	{
		draw_text("=", W_TITLE);
		move_x_to_len();
		draw_hex(*regval, 2, W_REGVAL);
	}

	next_row();
	draw_frame(w - 1, h);
	fill_rect(w - 1, h);
}

dbg_column& dbg_canvas::create_column(int w)
{
	auto x = 0;

	for_each(columns_.begin(), columns_.end(), [&x](dbg_column* item)
	{
		x += item->get_w();
	});

	const auto new_item = new dbg_column(w, x, *this);
	columns_.push_back(new_item);

	return *new_item;
}

void dbg_canvas::paint()
{
	for_each(columns_.begin(), columns_.end(), [](dbg_column* column)
	{
		column->paint();
	});
}

void dbg_canvas::add_item(dbg_control* control)
{
	if (&control->get_column() == nullptr)
		throw exception("column not assigned");

	controls_.push_back(control);
}