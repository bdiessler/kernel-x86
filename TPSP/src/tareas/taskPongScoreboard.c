#include "task_lib.h"

#define WIDTH TASK_VIEWPORT_WIDTH
#define HEIGHT TASK_VIEWPORT_HEIGHT

#define SHARED_SCORE_BASE_VADDR (PAGE_ON_DEMAND_BASE_VADDR + 0xF00)
#define CANT_PONGS 3


void task(void) {
	screen pantalla;
	// ¿Una tarea debe terminar en nuestro sistema?
	board_screen_init(pantalla);
	while (true)
	{

		// Completar:
		// - Pueden definir funciones auxiliares para imprimir en pantalla
		// - Pueden usar `task_print`, `task_print_dec`, etc. 

		//uint8_t my_id = ENVIRONMENT->task_id;
		//task_print_dec(pantalla, my_id, 2, 0, 0, C_FG_WHITE | C_BG_BLACK);
		// puntero a los scores guardados
		uint32_t *saved_scores = ((uint32_t*)(SHARED_SCORE_BASE_VADDR));
		for (uint8_t id = 0; id < CANT_PONGS; id++) // cantidad de partidas simultaneas
		{
			//print(pantalla, idGame * 2, ancho, altura + idGame*3, color, color)
			task_print_dec(pantalla, saved_scores[id*2], 2, WIDTH/2 -7, HEIGHT/2 - 2 + id*3, C_FG_CYAN | C_BG_BLACK);
			task_print_dec(pantalla, saved_scores[id*2+1], 2, WIDTH/2 +6, HEIGHT/2 - 2 + id*3, C_FG_MAGENTA | C_BG_BLACK);
		}
		
		
		syscall_draw(pantalla);
	}
}

void board_screen_init(screen pantalla){
	task_draw_box(pantalla, 5, 5, 28, 14, ' ', C_BG_RED);
	task_draw_box(pantalla, 5+1, 5+1, 28-2, 14-2, ' ', C_BG_BLACK);
	for (uint8_t i = 0; i < CANT_PONGS; i++)
	{
		task_print(pantalla, "Player 1", WIDTH/2 - 10, HEIGHT/2 - 3 + i*3, C_FG_CYAN|C_BG_BLACK);
		task_print(pantalla, "Player 2", WIDTH/2 + 3, HEIGHT/2 - 3 + i*3, C_FG_MAGENTA|C_BG_BLACK);
	}
	//task_draw_box(pantalla, WIDTH/2, HEIGHT/2, 1, 1, 'C', C_FG_WHITE | C_BG_BLUE);
	
}