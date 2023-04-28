#include "std.h"
#include "dbgpaint.h"
#include "dbgwidgets.h"
#include <algorithm>
#include <memory>

using namespace std;

dbg_control::dbg_control(const int h) :h_(h) { }

void dbg_control::check_parent() const
{
	if (column_ == nullptr || canvas_ == nullptr)
		throw exception("parent not assigned");
}

void dbg_control::draw_reg_frame(const char* title, const u8* value) const
{
	check_parent();
	canvas_->draw_reg_frame(*this, get_h(), title, value);
}

void dbg_control::draw_bits_range(const int bits) const
{
	check_parent();
	const auto color = is_active ? w_bits_active : w_bits;

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
			.draw_text(line, color)
			.move_x_to_len()
			.draw_text(":", color);
	}
	else
		(*canvas_).draw_text("   ", color);

	(*canvas_).next_col();
}

void dbg_control::draw_bit(const char* title, const int bits, const char* val_set[], const int val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm)
		.next_col()
		.draw_text("=", is_active ? w_bits_active : w_eq)
		.next_col();

	(*canvas_)
		.draw_text(val_set[val], is_active ? w_sel : w_norm);

	next_row();
}

void dbg_control::draw_bit(const char* title, const int bits, const u8 val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm)
		.next_col()
		.draw_text("=", is_active ? w_bits_active : w_eq)
		.next_col();

	(*canvas_)
		.draw_text((val & 1) ? "ena" : "dis", is_active ? w_sel : w_norm);
	next_row();
}

void dbg_control::draw_hex8_inline(const char* title, const u8 val) const
{
	check_parent();
	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text("= ", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_hex(val, 2, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(" ", is_active ? w_sel : w_norm).move_x_to_len();
}

void dbg_control::draw_dec_inline(const char* title, const u16 val) const
{
	check_parent();
	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text("= ", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_number(val, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(" ", is_active ? w_sel : w_norm).move_x_to_len();
}

void dbg_control::draw_hex16(const char* title, const int bits, const u16 val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm)
		.next_col()
		.draw_text("=", is_active ? w_bits_active : w_eq)
		.next_col();

	(*canvas_)
		.draw_hex(val, 4, is_active ? w_sel : w_norm);

	next_row();
}

void dbg_control::draw_hex24(const char* title, int bits, const u32 val) const
{
	check_parent();

	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm)
		.next_col()
		.next_col()
		.draw_text("=", is_active ? w_bits_active : w_eq)
		.next_col();

	(*canvas_)
		.draw_hex(val >> 14, 2, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(":", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_hex(val & 0x3FFF, 4, is_active ? w_sel : w_norm);

	next_row();
}

void dbg_control::draw_bit_d(const char* title, const int bits, const u8 val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm)
		.next_col()
		.draw_text("=", is_active ? w_bits_active : w_eq)
		.next_col();

	(*canvas_)
		.draw_number(val, is_active ? w_sel : w_norm);

	next_row();
}

void dbg_control::draw_bit_h(const char* title, const int bits, const u8 val) const
{
	check_parent();
	draw_bits_range(bits);

	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm)
		.next_col()
		.draw_text("=", is_active ? w_bits_active : w_eq)
		.next_col();

	(*canvas_)
		.draw_hex(val, 2, is_active ? w_sel : w_norm);

	next_row();
}

void dbg_control::draw_port(const char* title, const u8 val) const
{
	check_parent();

	(*canvas_)
		.draw_text(title, is_active ? w_sel : w_norm)
		.next_col()
		.next_col()
		.draw_text("=", is_active ? w_bits_active : w_eq)
		.next_col();

	(*canvas_)
		.draw_hex(val, 2, is_active ? w_sel : w_norm);

	next_row();
}

void dbg_control::draw_hl_port(const char prefix, const u8 hval, const u8 lval, const u16 val) const
{
	check_parent();

	char line[3] = "  ";
	char line1[2] = " ";
	line[0] = prefix;
	line1[0] = prefix;

	line[1] = 'H';
	(*canvas_)
		.draw_text(line, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text("= ", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_hex(hval, 2, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(" ", is_active ? w_sel : w_norm).move_x_to_len();

	line[1] = 'L';
	(*canvas_)
		.draw_text(line, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text("= ", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_hex(lval, 2, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(" ", is_active ? w_sel : w_norm).move_x_to_len();

	(*canvas_)
		.draw_text(line1, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text("= ", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_hex(val, 4, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(" ", is_active ? w_sel : w_norm).move_x_to_len();

	next_row();
}

void dbg_control::draw_xhl_port(const char prefix, const u8 xval, const u8 hval, const u8 lval) const
{
	check_parent();

	char line[3] = "  ";
	line[0] = prefix;

	line[1] = 'X';
	(*canvas_)
		.draw_text(line, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text("= ", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_hex(xval, 2, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(" ", is_active ? w_sel : w_norm).move_x_to_len();

	line[1] = 'H';
	(*canvas_)
		.draw_text(line, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text("= ", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_hex(hval, 2, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(" ", is_active ? w_sel : w_norm).move_x_to_len();

	line[1] = 'L';
	(*canvas_)
		.draw_text(line, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text("= ", is_active ? w_bits_active : w_eq).move_x_to_len()
		.draw_hex(lval, 2, is_active ? w_sel : w_norm).move_x_to_len()
		.draw_text(" ", is_active ? w_sel : w_norm).move_x_to_len();

	next_row();
}

void dbg_control::draw_led(const char* title, const u8 on) const
{
	check_parent();

	(*canvas_).move(1, 0);

	if (on & 1)
		(*canvas_).draw_text(title, w_ledon).move_x_to_len();
	else
		(*canvas_).move(static_cast<int>(strlen(title)), 0);
}

void dbg_control::set_xy(const int x, const int y) const
{
	canvas_->set_xy(column_->get_x() + x, get_y() + y);
}

void dbg_control::next_row() const
{
	canvas_->next_row();
}

void dbg_control::draw_text_on_line(const char* text, const int x, const u8 attr) const
{
	canvas_->set_x(column_->get_x() + x);
	canvas_->draw_text(text, attr);
}

int dbg_control::get_h() const
{
	return h_;
}

void dbg_control::set_y(const int y)
{
	y_ = y;
}

const dbg_column& dbg_control::get_column() const
{
	return *column_;
}

void dbg_control::set_parent(dbg_column* column, dbg_canvas* canvas, const int y)
{
	if (column_ != nullptr || canvas_ != nullptr)
		throw exception("already init");

	column_ = column;
	canvas_ = canvas;

	y_ = y;
}

bool dbg_control::handle_mouse(int mx, int my) const
{
	auto need_repaint = false;
	canvas_->iterate([this, &need_repaint](dbg_control* control)
		{
			const auto old = control->is_active;
			control->is_active = control == this;

			if (old != control->is_active)
				need_repaint = true;
		});

	return need_repaint;
}


void dbg_column::move_y(const int dy)
{
	y_ += dy;
}

dbg_column::dbg_column(const int w, const int x, dbg_canvas& canvas)
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

bool dbg_column::handle_mouse(const int mx, const int my) const
{
	for (const auto control : controls_)
	{
		const auto top = control->get_y();
		const auto bottom = control->get_y() + control->get_h();

		if (top <= my && my <= bottom)
			return control->handle_mouse(mx, my - top);
	}

	return false;
}

vector<dbg_control*> dbg_column::get_controls() const
{
	return controls_;
}


void dbg_canvas::print_dbg(const char* line, const int color)
{
	if (color == -1)
		tprint(base_x_ + x_, base_y_ + y_, line, attr_);
	else
		tprint(base_x_ + x_, base_y_ + y_, line, static_cast<u8>(color));

	last_len_ = strlen(line);
}

dbg_canvas::dbg_canvas(const int base_x, const int base_y)
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

dbg_canvas& dbg_canvas::set_cols(const int col0, const int col1, const int col2, const int col3)
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

dbg_canvas& dbg_canvas::draw_frame(const int w, const int h)
{
	frame(base_x_ + x_, base_y_ + y_, w, h, FRAME);
	return *this;
}

dbg_canvas& dbg_canvas::fill_rect(const int w, const int h, const u8 attr)
{
	fillrect(base_x_ + x_, base_y_ + y_, w, h, attr);

	return *this;
}

dbg_canvas& dbg_canvas::draw_text(const char* msg, const int color)
{
	char line[0x20];
	sprintf(line, "%s", msg);
	print_dbg(line, color);

	return *this;
}

dbg_canvas& dbg_canvas::draw_number(const int value, const int color)
{
	char line[0x20];
	sprintf(line, "%03d", value);
	print_dbg(line, color);

	return *this;
}

dbg_canvas& dbg_canvas::draw_hex(const unsigned value, const int len, const int color)
{
	char line[0x20];
	char fstr[5] = "%0_X";
	fstr[2] = static_cast<char>(48 + len);

	sprintf(line, fstr, value);
	print_dbg(line, color);

	return *this;
}

dbg_canvas& dbg_canvas::move(const int dx, const int dy)
{
	x_ += dx;
	y_ += dy;

	return *this;
}

dbg_canvas& dbg_canvas::set_x(const int x)
{
	x_ = x;

	return *this;
}

int dbg_canvas::get_x() const
{
	return base_x_;
}

int dbg_canvas::get_y() const
{
	return base_y_;
}

void dbg_canvas::draw_reg_frame(const dbg_control& control, const int h, const char* title, const u8* regval)
{
	const auto x = control.get_column().get_x();
	const auto y = control.get_y();
	const auto w = control.get_column().get_w();

	set_cols(x + 0, x + 3, x + 12, x + 14);
	set_xy(x, y);

	draw_text(title, w_title);
	move_x_to_len();

	if (regval != nullptr)
	{
		draw_text("=", w_title);
		move_x_to_len();
		draw_hex(*regval, 2, w_regval);
	}

	next_row();
	draw_frame(w - 1, h);
	fill_rect(w - 1, h, control.is_active ? w_sel : w_norm);
}

dbg_column& dbg_canvas::create_column(const int w)
{
	auto x = 0;

	for_each(columns_.begin(), columns_.end(), [&x](const dbg_column* item)
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

bool dbg_canvas::handle_mouse(const int mx, const int my) const
{
	for (const auto column : columns_)
	{
		const auto left = column->get_x();
		const auto right = column->get_x() + column->get_w();

		if (left <= mx && mx <= right)
			return column->handle_mouse(mx - left, my);
	}

	return false;
}

void dbg_canvas::add_item(dbg_control* control)
{
	if (&control->get_column() == nullptr)
		throw exception("column not assigned");

	controls_.push_back(control);
}

void dbg_canvas::iterate(const function<void(dbg_control*)>& functor) const
{
	for (const auto column : columns_)
		for (const auto control : column->get_controls())
			functor(control);
}
