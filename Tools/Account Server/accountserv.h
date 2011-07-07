typedef struct player_s player_t;

struct player_s {
	player_t		*prev;
	player_t		*next;
	
	char			name[32];
	int				time;
};
extern player_t players;
extern void DumpValidPlayersToFile(void);
extern bool ValidatePlayer(char name[32], char password[32]);