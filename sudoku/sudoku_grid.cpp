#include "sudoku_grid.h"

SudokuGrid::SudokuGrid(int cell_size, QWidget *parent)
	: QLabel(parent),
	  cell_size(cell_size),
	  cell_span(cell_size * cell_size),
	  fixed_size(50),
	  current_selected(nullptr),
	  sudoku(std::make_shared<Sudoku>(cell_size)),
	  actions(10)
{
	// init cells
	top_layer = new QGridLayout(this);

	cells.assign(cell_span * cell_span, nullptr);
	for(int r = 0; r != cell_size; ++r)
		for(int c = 0; c != cell_size; ++c)
		{
			QGridLayout *grid = new QGridLayout;
			top_layer->addLayout(grid, r, c);
			grid->setSpacing(1);

			for(int x = 0; x != cell_size; ++x)
				for(int y = 0; y != cell_size; ++y)
				{
					int row = r * cell_size + x;
					int col = c * cell_size + y;
					SudokuCell *cell = new SudokuCell(row, col, sudoku);
					cell->setFocusPolicy(Qt::StrongFocus);
					cell->setFixedSize(fixed_size, fixed_size);
					// cell->setWordWrap(true);
					cell->setAlignment(Qt::AlignCenter);

					grid->addWidget(cell, x, y);

					cells[row * cell_span + col] = cell;
				}

			grids.push_back(grid);
		}

	// connect signals
	for(int r = 0; r != cell_span; ++r)
		for(int c = 0; c != cell_span; ++c)
		{
			SudokuCell *cell = cells[r * cell_span + c];
			connect(cell, SIGNAL(selected_signal(SudokuCell*)),
					this, SLOT(cell_selected(SudokuCell*)));

			connect(cell, SIGNAL(value_changed(int,int,int, IntList)),
					this, SLOT(value_changed(int,int,int, IntList)));

			for(int i = 0; i != cell_span; ++i)
			{
				connect(cell, SIGNAL(selected_signal(SudokuCell*)),
						cells[i * cell_span + c], SLOT(vertical_selected()));

				if(i != r) connect(cell, SIGNAL(free_signal()),
								   cells[i * cell_span + c], SLOT(free_selection()));

				connect(cell, SIGNAL(selected_signal(SudokuCell*)),
						cells[r * cell_span + i], SLOT(horizontal_selected()));

				if(i != c) connect(cell, SIGNAL(free_signal()),
								   cells[r * cell_span + i], SLOT(free_selection()));
			}
		}

	// set layer style
	top_layer->setSpacing(2);
	top_layer->setMargin(2);
	setStyleSheet("background-color: #666;");

	int size = (fixed_size + 1) * cell_span + cell_size + 2;
	setFixedSize(size, size);
}

SudokuGrid::~SudokuGrid()
{
}

void SudokuGrid::cell_selected(SudokuCell *cell)
{
	if(cell != current_selected)
	{
		if(current_selected)
			current_selected->free_selection();
		current_selected = cell;
	}
}

void SudokuGrid::add_value(int v)
{
	if(current_selected)
		current_selected->add_value(v);
}

void SudokuGrid::remove_value(int v)
{
	if(current_selected)
		current_selected->remove_value(v);
}

void SudokuGrid::game_start()
{
	// TODO: Level selection
	if(current_selected)
		current_selected->free_selection();

	Sudoku new_sudoku = *sudoku;
	new_sudoku.random_sudoku(11, 50, 0);
	*sudoku = new_sudoku;

	for(int r = 0; r != cell_span; ++r)
		for(int c = 0; c != cell_span; ++c)
		{
			int id = r * cell_span + c;
			cells[id]->set_initial_status(sudoku->get(r, c));
		}
}


void SudokuGrid::game_hint()
{
	Sudoku hint_sudoku = sudoku->solve();
	if(hint_sudoku.is_empty())
	{
		QMessageBox::warning(this, "No Solution!", "Your current status leads to no solution.");
	} else {
		IntList empty_cells;
		for(int r = 0; r != cell_span; ++r)
			for(int c = 0; c != cell_span; ++c)
				if(sudoku->get(r, c) == 0)
					empty_cells.push_back(r * cell_span + c);

		if(empty_cells.empty())
		{
			// already solved
			return;
		}

		int id = empty_cells[std::rand() % empty_cells.size()];
		int r = id / cell_span, c = id % cell_span;
		cells[id]->set_hint_value(hint_sudoku.get(r, c));
	}
}

void SudokuGrid::clear_grid()
{
	if(current_selected)
		current_selected->clear_values();
}

void SudokuGrid::value_changed(int r, int c, int v, IntList candidates)
{
	actions.add_action(r, c, v, candidates);
}

void SudokuGrid::backward_step()
{
	ActionInfo action = actions.backward();

	if(action.row < 0)
		return;

	int id = action.row * cell_span + action.col;
	if(action.value < 0)
	{
		cells[id]->add_value(-action.value, false);
	} else if(action.value == 0) {
		for(int v = 1; v <= cell_span; ++v)
			if(action.candidates[v])
				cells[id]->add_value(v, false);
	} else {
		cells[id]->remove_value(action.value, false);
	}
}

void SudokuGrid::forward_step()
{
	ActionInfo action = actions.forward();

	if(action.row < 0)
		return;

	int id = action.row * cell_span + action.col;
	if(action.value > 0)
	{
		cells[id]->add_value(action.value, false);
	} else if(action.value == 0) {
		cells[id]->clear_values(false);
	} else {
		cells[id]->remove_value(-action.value, false);
	}
}