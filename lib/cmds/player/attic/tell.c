/* Do not remove the headers from this file! see /USAGE for more info. */

// /* Do not remove the headers from this file! see /USAGE for more info. */

//:COMMAND
//
// USAGE: tell <player> <message>
//        tell <player>@<mudname> <message>
//        tell /last
//        tell /clear
//
// This command is used to tell others private messages. The second format
// can be used to tell to people on other muds. The /last syntax will
// display your tell history and the /clear syntax will clear it.

#include <mudlib.h>
#include <commands.h>

inherit CMD;
inherit M_GRAMMAR;
inherit M_COMPLETE;
inherit M_ANSI;

#define MAX_HISTORY 20

void query_history(string name);
void add_history(string name, string msg);
void clear_history(string name);

private mapping history = ([ ]);

void create()
{
    ::create();
    no_redirection();
}

private void main(string arg)
{
    string user;
    string host;
    mixed tmp;
    string *words;
    string muds;
    string *previous_matches;
    string *matches;
    int i, j;
    string mystring;
    string deststring;
    object who;

    if(!arg)
    {
	out("Usage: tell <user> <message>\n");
	return;
    }

    if (arg == "/last")
	return query_history(this_user()->query_userid());

    if (arg == "/clear")
	return clear_history(this_user()->query_userid());


    if(sscanf(arg,"%s@%s", user, tmp) == 2) {
	muds = IMUD_D->query_up_muds();
	words = explode(tmp, " ");
	j = sizeof(words);
	tmp = "";
	for(i=0;i<j;i++)
	{
	    tmp += " " + words[i];
	    if(tmp[0] == ' ')
		tmp = tmp[1..];
	    matches = find_best_match_or_complete(tmp, muds);
	    if(!sizeof(matches))
	    {
		break;
	    }
	    previous_matches = matches;
	}
	if(previous_matches)
	{
	    if(sizeof(previous_matches) > 1)
	    {
		out("Vague mud name.  could be: " 
		  + implode(previous_matches, ", ") + "\n");
		return;
	    }                


	    host = previous_matches[0];
	    arg  = implode(words[i..], " ");
	    if(host == mud_name())
	    {
		main(user+" "+arg);
		return;
	    }

	    if( arg[0] == ';' || arg[0] == ':' )
	      {
		mixed *soul_ret;
		
		arg = arg[1..];
		soul_ret = SOUL_D->parse_imud_soul(arg);
		
		if(!soul_ret)  {
		  IMUD_D->do_emoteto(host, user, sprintf("%s@%s %s",capitalize(user),host,arg));
                    outf("%%^TELL%%^You emote to %s@%s:%%^RESET%%^ %s %s\n", capitalize(user), host, this_body()->query_name(), arg);
                    add_history(this_user()->query_userid(),
                                sprintf("%%^TELL%%^You emote to %s@%s:%%^RESET%%^ %s %s\n",
                                        capitalize(user), host, this_body()->query_name(), arg));
		    return;
		}
		IMUD_D->do_emoteto(host,user,soul_ret[1][<1]);
		outf("%%^TELL%%^(tell)%%^RESET%%^ %s", soul_ret[1][0]);
		add_history(this_user()->query_userid(),
		            sprintf("%%^TELL%%^(tell)%%^RESET%%^ %s", soul_ret[1][0]));
		return;
	    }
	    IMUD_D->do_tell(host, user, arg);
            outf("%%^TELL%%^You tell %s@%s: %%^RESET%%^%s\n", capitalize(user), host, arg);
            add_history(this_user()->query_userid(),
                        sprintf("%%^TELL%%^You tell %s@%s: %%^RESET%%^%s\n",
                                capitalize(user), host, arg));
	    return;
	}
    }
    if(sscanf(arg, "%s %s", user, arg) != 2)
    {
	out("Usage: tell <user> <message>\n");
	return;
    }
    who = find_body(lower_case(user));
    if(!who)
    {
	outf("Couldn't find %s.\n", user);
	return;
    }

    if (who->query_invis() && !adminp(this_user()) )
    {
	outf("Couldn't find %s.\n", user);
	return;
    }
    if (!who->query_link() || !interactive(who->query_link()))
    {
	outf("%s is linkdead.\n", who->query_name());
	return;
    }

    if( arg[0] == ':' || arg[0] == ';' )
    {
	mixed *soul_ret;
	int tindex;

	arg = arg[1..];

	soul_ret = SOUL_D->parse_soul(arg);
	if(!soul_ret)  {
            mystring = sprintf("%%^TELL%%^You emote to %s: %%^RESET%%^%s %s\n", who == this_body() ? "yourself" : who->query_name(), this_body()->query_name(),arg);
	    deststring = sprintf("*%s %s\n", this_body()->query_name(), arg);
	}
	else
	{
	    mystring = sprintf("%%^TELL%%^(tell)%%^RESET%%^ %s", soul_ret[1][0]);

	    if((tindex = member_array(who, soul_ret[0])) == -1)  {
		deststring = sprintf("%%^TELL%%^(tell)%%^RESET%%^ %s", soul_ret[1][<1]);
	    }
	    else
	    {
		deststring = sprintf("%%^TELL%%^(tell)%%^RESET%%^ %s", soul_ret[1][tindex]);
	    }
	}
    }
    else
    {
	mystring = sprintf("%%^TELL%%^You tell %s:%%^RESET%%^ %s\n", who == this_body() ? "yourself" : who->query_name(), arg);
	deststring = "%^TELL%^" + this_body()->query_name() + " tells you: %^RESET%^" + arg + "\n";
    }

    out(mystring);
    add_history(this_user()->query_userid(), mystring);
    if(who != this_body())
    {
	who->receive_private_msg(deststring, MSG_INDENT);
	add_history(who->query_userid(), deststring);
	who->set_reply(this_user()->query_userid());
    }
}

void add_history(string name, string msg)
{
  if( !this_user() )
  {
    if( find_object(IMUD_D) != previous_object() )
      error("Illegal attempt to modify 'tell' history.");
  }

  if (!history[name])
    history[name] = ({ msg });
  else
    history[name] += ({ msg });

  if (sizeof(history[name]) > MAX_HISTORY)
    history[name] = history[name][1..<1];
}

private void query_history(string name)
{
   string hist = history[name] ? implode(history[name], "") : "";

  if ( hist == "" )
    hist = "<none>\n";

  more(sprintf("History of 'tells':\n%s\n", hist));
}

private void clear_history(string name)
{
  map_delete(history, name);
}

nomask int
valid_resend(string ob) {
    return ob == "/cmds/player/reply";
}
