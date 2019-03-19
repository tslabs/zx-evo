// кря
#pragma once
#include "sysdefs.h"
#include <vector>
#include <functional>


using namespace std;

#define	W_REGVAL		0x51
#define	W_BITS			0x04
#define	W_EQ			0x04
#define	W_BITS_ACTIVE	0x14
#define	W_EQ_ACTIVE		0x14

//#define W_LEDOFF		0x50
#define W_LEDON 		0x50

class dbg_column;
class dbg_canvas;

class dbg_control
{
	int h_ = 0;
	int y_ = 0;	
	dbg_column* column_ = nullptr;
	dbg_canvas* canvas_ = nullptr;	

	void check_parent() const;	

protected:
	void draw_reg_frame(const char *title, const u8 *value = nullptr) const;
	void draw_bits_range(int bits) const;
	void draw_bit(char* title, int bits, const char* val_set[], int val) const;
	void draw_bit(char* title, int bits, u8 val) const;
	void draw_hex8_inline(char* title, u8 val) const;
	void draw_dec_inline(char* title, u16 val) const;
	void draw_hex16(char* title, int bits, u16 val) const;
	void draw_hex24(char* title, int bits, u32 val) const;
	void draw_bit_d(char* title, int bits, u8 val) const;
    void draw_bit_h(char * title, int bits, u8 val) const;
	void draw_port(char* title, u8 val) const;
	void draw_hl_port(char prefix, u8 hval, u8 lval, u16 val) const;
	void draw_xhl_port(char prefix, u8 xval, u8 hval, u8 lval) const;
	void draw_led(char* title, u8 on) const;
	void set_xy(int x, int y) const;
	void next_row() const;
	void draw_text_on_line(const char* text, int x, u8 attr) const;

public:
	bool is_active{};

	virtual ~dbg_control() = default;
	explicit dbg_control(int h);

	int get_h() const;
	int get_y() const { return y_; }
	const dbg_column& get_column() const;

	void set_y(int y);
	void set_parent(dbg_column *column, dbg_canvas *canvas, int y);

	bool handle_mouse(int mx, int my);

	virtual void on_paint() {};
};

class dbg_column
{
	int y_ = 0;
	const int w_;
	const int x_;

	vector<dbg_control*> controls_ = vector<dbg_control*>();
	dbg_canvas &canvas_;
public:
	dbg_column(int w, int x, dbg_canvas& canvas);

	int get_w() const;
	int get_x() const;
	int get_y() const;
	
	void reset();
	void move_y(int dy);

	void add_item(dbg_control * control)
	{
		controls_.push_back(control);

		control->set_parent(this, &canvas_, get_y());
		move_y(control->get_h() + 1);
	}
	
	void paint();
	bool handle_mouse(int mx, int my);
	vector<dbg_control*> get_controls() const;
};

class dbg_canvas final
{
	u8 attr_ = 0;
	int base_x_;
	int base_y_;
	int x_ = 0;
	int y_ = 0;
	int cols_[4] = {-1, -1, -1, -1};
	int curr_col_ = 0;
	int last_len_ = 0;

	vector<dbg_column*> columns_ = vector<dbg_column*>();
	vector<dbg_control*> controls_ = vector<dbg_control*>();

	void print_dbg(char* line, int color);

public:
	dbg_canvas(int base_x, int base_y);
	~dbg_canvas();

	dbg_canvas& set_attr(const u8 color);
	dbg_canvas& set_xy(int x, int y);
	dbg_canvas& set_cols(int col0 = -1, int col1 = -1, int col2 = -1, int col3 = -1);
	dbg_canvas& next_row();
	dbg_canvas& next_col();
	dbg_canvas& move_x_to_len();
	dbg_canvas& draw_frame(const int w, int h);
	dbg_canvas& fill_rect(const int w, const int h, u8 attr);
	dbg_canvas& draw_text(const char *msg, int color = -1);
	dbg_canvas& draw_number(const int value, int color = -1);
	dbg_canvas& draw_hex(const unsigned value, int len = 2, int color = -1);

	dbg_canvas& move(int dx, int dy);
	dbg_canvas& set_x(int x);

	int get_x() const;
	int get_y() const;

	void draw_reg_frame(const dbg_control& control, int h, const char* title, const u8 *regval = nullptr);

	dbg_column& create_column(int w);
	void add_item(dbg_control* control);
	void iterate(function<void(dbg_control*)> functor);
	
	void paint();
	bool handle_mouse(int mx, int my);
	
};
