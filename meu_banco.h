#ifndef MEU_BANCO_H
#define MEU_BANCO_H

#include <stdlib.h>

#define ARQUIVO_BANCO "dados_banco.txt"
#define MAX_HISTORICO 5
#define NUMERO_CONTA_INICIAL 1000
#define QTD_FINALISTAS_SORTEIO 5

typedef struct {
    double valor;
    char op; // 'S' - saque, 'D' - deposito, 'E' - emprestimo
} Transacao;

typedef struct {
    char cliente[100];
    int numero;               // 4 digitos, iniciando em 1000 (ex: 1000)
    int tem_corrente;         // 01: 1 = sim, 0 = nao
    int tem_salario;          // 02: 1 = sim, 0 = nao
    int tem_poupanca;         // 03: 1 = sim, 0 = nao
    double saldo_corrente;
    double saldo_salario;
    double saldo_poupanca;
    int emprestimo_realizado; // 1 = sim, 0 = nao (apenas 1 por cliente)
    double valor_emprestimo;
    int possui_titulo;        // 1 = sim, 0 = nao
    int contemplado_sorteio;  // 1 = sim, 0 = nao
    double valor_premio;
    int conta_premio;         // 1 = corrente, 2 = salario, 3 = poupanca
    Transacao historico[MAX_HISTORICO];
    int qtd_historico;
} Carteira;

typedef struct {
    Carteira *clientes; // Ponteiro alocado dinamicamente para n clientes
    int quantidade;
    int capacidade;
    int proximo_numero; // Contagem iniciando em 1000
    int aberto;         // 1 = aberto, 0 = fechado
} Banco;

// Gerenciamento de memoria e utilitarios
void inicializar_banco(Banco *banco);
void liberar_banco(Banco *banco);
void limpar_buffer(void);
int ler_linha(char *dest, size_t tamanho);
int fibonacci(int n);

// Visao do Banco
void abrir_banco(Banco *banco);
void cadastrar_cliente(Banco *banco);
void consultar_historico_banco(const Banco *banco);
void efetuar_sorteio(Banco *banco);

// Visao do Cliente
void acessar_conta_cliente(Banco *banco);
void menu_cliente(Banco *banco, Carteira *cliente, int tipo_conta);
void realizar_deposito(Banco *banco, Carteira *cliente, int tipo_conta);
void realizar_saque(Banco *banco, Carteira *cliente, int tipo_conta);
void solicitar_emprestimo(Banco *banco, Carteira *cliente);
void adquirir_titulo_capitalizacao(Banco *banco, Carteira *cliente);
void solicitar_extrato(const Carteira *cliente);

// Calculos, historico e persistencia em arquivo
void registrar_transacao(Carteira *c, char op, double valor);
double calcular_saldo_total_cliente(const Carteira *c);
double obter_saldo_conta(const Carteira *c, int tipo_conta);
void salvar_dados_arquivo(const Banco *banco);
void carregar_dados_arquivo(Banco *banco);

#endif
