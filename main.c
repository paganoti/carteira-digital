#include <stdio.h>
#include <string.h>
#include <time.h>
#include "meu_banco.h"

int main(void) {
    srand((unsigned int)time(NULL));

    Banco banco;
    inicializar_banco(&banco);

    char opcao[32];

    while (1) {
        printf("\n=== MENU PRINCIPAL - CARTEIRA DIGITAL ===\n");
        printf("Status do banco: %s\n", banco.aberto ? "ABERTO" : "FECHADO");
        printf("[1] Abrir o banco\n");
        printf("[2] Consultar historico\n");
        printf("[3] Efetuar sorteio\n");
        printf("[4] Cadastrar cliente\n");
        printf("[5] Acessar conta (Visao do Cliente)\n");
        printf("[s] Sair\n");
        printf("Opcao: ");

        if (!ler_linha(opcao, sizeof(opcao))) {
            printf("\nFim da entrada detectado. Encerrando...\n");
            if (banco.aberto == 1) {
                salvar_dados_arquivo(&banco);
            }
            break;
        }

        if (strcmp(opcao, "1") == 0) {
            abrir_banco(&banco);
        } else if (strcmp(opcao, "2") == 0) {
            consultar_historico_banco(&banco);
        } else if (strcmp(opcao, "3") == 0) {
            efetuar_sorteio(&banco);
        } else if (strcmp(opcao, "4") == 0) {
            cadastrar_cliente(&banco);
        } else if (strcmp(opcao, "5") == 0) {
            acessar_conta_cliente(&banco);
        } else if (strcmp(opcao, "s") == 0 || strcmp(opcao, "S") == 0 || strcmp(opcao, "0") == 0) {
            if (banco.aberto == 1) {
                salvar_dados_arquivo(&banco);
                printf("\nDados salvos em '%s'. Finalizando programa...\n", ARQUIVO_BANCO);
            } else {
                printf("\nFinalizando programa...\n");
            }
            break;
        } else {
            printf("\n[ERRO] Opcao invalida! Digite 's' ou '0' para sair.\n");
        }
    }

    liberar_banco(&banco);
    return 0;
}
