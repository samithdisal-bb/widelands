/*
 * Copyright (C) 2002 by the Widelands Development Team
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "widelands.h"
#include "graphic.h"
#include "input.h"
#include "ui.h"
#include "fileloc.h"
#include "cursor.h"
#include "font.h"
#include "mainmenue.h"
#include "menuecommon.h"
#include "game.h"
#include "mapselectmenue.h"
#include "map.h"
#include "player.h"

/*
==============================================================================

PlayerDescriptionGroup

==============================================================================
*/

/** class PlayerDescriptionGroup
 *
 * - checkbox to enable/disable player
 * - button to switch between: Human, Remote, AI
 */
class PlayerDescriptionGroup : public Panel {
	Game* m_game;
	int m_plnum;
	
	bool m_enabled; // is this player allowed at all (map-dependent)
	
	Checkbox* m_btnEnablePlayer;
	int m_playertype;
	Button* m_btnPlayerType;

public:
	PlayerDescriptionGroup(Panel* parent, int x, int y, Game* game, int plnum);

	UISignal changed;
		
	void set_enabled(bool enable);

private:	
	void enable_player(bool on);
	void toggle_playertype();
};

PlayerDescriptionGroup::PlayerDescriptionGroup(Panel* parent, int x, int y, Game* game, int plnum)
	: Panel(parent, x, y, 300, 20)
{
	m_game = game;
	m_plnum = plnum;
	
	m_enabled = false;
	set_visible(false);

	// create sub-panels
	m_btnEnablePlayer = new Checkbox(this, 0, 0);
	m_btnEnablePlayer->set_state(true);
	m_btnEnablePlayer->changedto.set(this, &PlayerDescriptionGroup::enable_player);
	
	m_btnPlayerType = new Button(this, 28, 0, 174, 20, 1);
	m_btnPlayerType->clicked.set(this, &PlayerDescriptionGroup::toggle_playertype);
	if (!plnum)
		m_playertype = Player::playerLocal;
	else
		m_playertype = Player::playerAI;
}

/** PlayerDescriptionGroup::set_enabled(bool enable)
 *
 + The group is enabled if the map has got a starting position for this player.
 * We need to update the Game class accordingly.
 */
void PlayerDescriptionGroup::set_enabled(bool enable)
{
	if (enable == m_enabled)
		return;
	
	m_enabled = enable;
	
	if (!m_enabled)
	{
		if (m_btnEnablePlayer->get_state())
			m_game->remove_player(m_plnum);
		set_visible(false);
	}
	else
	{
		if (m_btnEnablePlayer->get_state())
			m_game->add_player(m_plnum, m_playertype);
			
		const char* string = 0;
		switch(m_playertype) {
		case Player::playerLocal: string = "Human"; break;
		case Player::playerAI: string = "Computer"; break;
		}
		m_btnPlayerType->set_pic(g_fh.get_string(string, 0));
		m_btnPlayerType->set_visible(m_btnEnablePlayer->get_state());
		
		set_visible(true);
	}
	
	changed.call();
}

/** PlayerDescriptionGroup::enable_player(bool on)
 *
 * Update the Game when the checkbox is changed.
 */
void PlayerDescriptionGroup::enable_player(bool on)
{
	if (on) {
		m_game->add_player(m_plnum, m_playertype);
	} else {
		m_game->remove_player(m_plnum);
	}

	m_btnPlayerType->set_visible(on);
	changed.call();
}

void PlayerDescriptionGroup::toggle_playertype()
{
	// NOOP: toggling the player type is currently not possible
}

/*
==============================================================================

LaunchGameMenu

==============================================================================
*/

class LaunchGameMenu : public BaseMenu {
	Game* m_game;

	Button*						m_ok;
	Textarea*					m_mapname;
	PlayerDescriptionGroup* m_players[MAX_PLAYERS];
	
public:
	LaunchGameMenu(Game *g);

	void think();

	void refresh();

	void select_map();
};

LaunchGameMenu::LaunchGameMenu(Game *g)
	: BaseMenu("splash.bmp")
{
	m_game = g;

	// Title
	new Textarea(this, MENU_XRES/2, 140, "Launch game", Textarea::H_CENTER);

	// Buttons
	Button* b;

	b = new Button(this, 410, 406, 174, 24, 0, 0);
	b->clickedid.set(this, &LaunchGameMenu::end_modal);
	b->set_pic(g_fh.get_string("Back", 0));

	m_ok = new Button(this, 410, 436, 174, 24, 2, 1);
	m_ok->clickedid.set(this, &LaunchGameMenu::end_modal);
	m_ok->set_pic(g_fh.get_string("Start game", 0));
	m_ok->set_enabled(false);

	// Map selection fields
	m_mapname = new Textarea(this, 497, 184, "(no map)", Textarea::H_CENTER);
	b = new Button(this, 410, 200, 174, 24, 1, 0);
	b->clicked.set(this, &LaunchGameMenu::select_map);
	b->set_pic(g_fh.get_string("Select map", 0));
	
	// Player settings
	int i;
	int y;
	
	y = 184;
	for(i = 0; i < MAX_PLAYERS; i++)	{
		PlayerDescriptionGroup *pdg = new PlayerDescriptionGroup(this, 30, y, m_game, i);
		pdg->changed.set(this, &LaunchGameMenu::refresh);
		
		m_players[i] = pdg;
		y += 30;
	}
}

void LaunchGameMenu::think()
{
	m_game->think();
}

void LaunchGameMenu::refresh()
{
	const char *mapname = m_game->get_mapname();
	int maxplayers = 0;
	
	// update the mapname
	if (mapname) {
		Map m;
		if (m.load_map_header(mapname) != RET_OK)
		{
			m_game->set_mapname(0);
			m_mapname->set_text("(invalid map)");
		}
		else
		{
			m_mapname->set_text(m.get_name());
			maxplayers = m.get_nplayers();
		}
	} else {
		m_mapname->set_text("(no map)");
	}
	
	// update the player description groups
	int i;
	for(i = 0; i < MAX_PLAYERS; i++)
		m_players[i]->set_enabled(i < maxplayers);
	
	m_ok->set_enabled(m_game->can_start());
}

void LaunchGameMenu::select_map()
{
	map_select_menue(m_game);
	refresh();
}

/** void launch_game_menu(void)
 *
 * In the Launch Game menu, players can set the game settings, go to
 * map selection etc..
 */
bool launch_game_menu(Game *g)
{
	LaunchGameMenu *lgm = new LaunchGameMenu(g);
	int code = lgm->run();
	delete lgm;

	return (code > 0) ? true : false;
}
