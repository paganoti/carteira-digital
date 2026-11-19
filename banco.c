#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "meu_banco.h"

static double arredondar_centavos(double x) {
    double r = round(x * 100.0) / 100.0;
    if (fabs(r) < 0.005) {
        return 0.0; // Evita -0.00
    }
    return r;
}

void limpar_buffer(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        // Descarta caracteres restantes na entrada
    }
}

// Le uma linha de stdin com seguranca, remove espacos nas extremidades e retorna 0 em EOF
int ler_linha(char *dest, size_t tamanho) {
    if (dest == NULL || tamanho == 0) {
        return 0;
    }

    if (fgets(dest, (int)tamanho, stdin) == NULL) {
        dest[0] = '\0';
        return 0;
    }

    size_t len = strlen(dest);
    if (len > 0 && dest[len - 1] == '\n') {
        dest[len - 1] = '\0';
    } else if (!feof(stdin)) {
        limpar_buffer();
    }

    // Remove espacos/quebras de linha do final
    len = strlen(dest);
    while (len > 0 && isspace((unsigned char)dest[len - 1])) {
        dest[--len] = '\0';
    }

    // Remove espacos do inicio
    size_t inicio = 0;
    while (dest[inicio] != '\0' && isspace((unsigned char)dest[inicio])) {
        inicio++;
    }
    if (inicio > 0) {
        memmove(dest, dest + inicio, len - inicio + 1);
    }

    return 1;
}

// Retorna 1 se valido, 0 se invalido, -1 se EOF
static int ler_inteiro(int *dest) {
    char buf[64];
    if (!ler_linha(buf, sizeof(buf))) {
        return -1;
    }
    if (buf[0] == '\0') {
        return 0;
    }

    errno = 0;
    char *fim = NULL;
    long val = strtol(buf, &fim, 10);
    if (fim == buf || *fim != '\0' || errno == ERANGE || val < INT_MIN || val > INT_MAX) {
        return 0;
    }

    *dest = (int)val;
    return 1;
}

// Retorna 1 se numerico finito valido, 0 se invalido (texto, nan, inf, parcial), -1 se EOF
static int ler_valor_monetario(double *dest) {
    char buf[64];
    if (!ler_linha(buf, sizeof(buf))) {
        return -1;
    }
    if (buf[0] == '\0') {
        return 0;
    }

    errno = 0;
    char *fim = NULL;
    double val = strtod(buf, &fim);
    if (fim == buf || *fim != '\0' || errno == ERANGE || !isfinite(val) || fabs(val) > 1e13) {
        return 0;
    }

    *dest = arredondar_centavos(val);
    return 1;
}

void inicializar_banco(Banco *banco) {
    banco->capacidade = 5;
    banco->quantidade = 0;
    banco->proximo_numero = NUMERO_CONTA_INICIAL;
    banco->aberto = 0;
    banco->clientes = (Carteira *)malloc((size_t)banco->capacidade * sizeof(Carteira));
    if (banco->clientes == NULL) {
        printf("[ERRO] Falha ao alocar memoria com malloc.\n");
        banco->capacidade = 0;
    }
}

void liberar_banco(Banco *banco) {
    if (banco->clientes != NULL) {
        free(banco->clientes);
        banco->clientes = NULL;
    }
    banco->quantidade = 0;
    banco->capacidade = 0;
    banco->aberto = 0;
}

static int expandir_capacidade(Banco *banco) {
    int nova_capacidade = (banco->capacidade > 0) ? (banco->capacidade * 2) : 5;
    Carteira *novo_vetor = (Carteira *)realloc(banco->clientes, (size_t)nova_capacidade * sizeof(Carteira));
    if (novo_vetor == NULL) {
        printf("[ERRO] Falha ao realocar memoria para novos clientes.\n");
        return 0;
    }
    banco->clientes = novo_vetor;
    banco->capacidade = nova_capacidade;
    return 1;
}

int fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1 || n == 2) return 1;
    int a = 1, b = 1;
    for (int i = 3; i <= n; i++) {
        int prox = a + b;
        a = b;
        b = prox;
    }
    return b;
}

double calcular_saldo_total_cliente(const Carteira *c) {
    return arredondar_centavos(c->saldo_corrente + c->saldo_salario + c->saldo_poupanca);
}

double obter_saldo_conta(const Carteira *c, int tipo_conta) {
    if (tipo_conta == 1) return c->saldo_corrente;
    if (tipo_conta == 2) return c->saldo_salario;
    if (tipo_conta == 3) return c->saldo_poupanca;
    return 0.0;
}

static const char *nome_tipo_conta(int tipo_conta) {
    if (tipo_conta == 1) return "Corrente";
    if (tipo_conta == 2) return "Salario";
    if (tipo_conta == 3) return "Poupanca";
    return "Desconhecida";
}

// Registra as ultimas 5 transacoes (OP: S - saque, D - deposito, E - emprestimo)
void registrar_transacao(Carteira *c, char op, double valor) {
    Transacao nova;
    nova.valor = arredondar_centavos(valor);
    nova.op = op;

    if (c->qtd_historico < MAX_HISTORICO) {
        c->historico[c->qtd_historico] = nova;
        c->qtd_historico++;
    } else {
        for (int i = 0; i < MAX_HISTORICO - 1; i++) {
            c->historico[i] = c->historico[i + 1];
        }
        c->historico[MAX_HISTORICO - 1] = nova;
    }
}

void salvar_dados_arquivo(const Banco *banco) {
    FILE *fp = fopen(ARQUIVO_BANCO, "w");
    if (fp == NULL) {
        printf("[ERRO] Nao foi possivel abrir o arquivo '%s' para escrita.\n", ARQUIVO_BANCO);
        return;
    }

    fprintf(fp, "%d %d\n", banco->quantidade, banco->proximo_numero);
    for (int i = 0; i < banco->quantidade; i++) {
        const Carteira *c = &banco->clientes[i];
        fprintf(fp, "%s\n", c->cliente);
        fprintf(fp, "%d %d %d %d %.2f %.2f %.2f %d %.2f %d %d %.2f %d %d\n",
                c->numero, c->tem_corrente, c->tem_salario, c->tem_poupanca,
                c->saldo_corrente, c->saldo_salario, c->saldo_poupanca,
                c->emprestimo_realizado, c->valor_emprestimo,
                c->possui_titulo, c->contemplado_sorteio,
                c->valor_premio, c->conta_premio, c->qtd_historico);

        for (int j = 0; j < c->qtd_historico; j++) {
            fprintf(fp, "%.2f %c\n", c->historico[j].valor, c->historico[j].op);
        }
    }

    fclose(fp);
}

void carregar_dados_arquivo(Banco *banco) {
    FILE *fp = fopen(ARQUIVO_BANCO, "r");
    if (fp == NULL) {
        banco->quantidade = 0;
        banco->proximo_numero = NUMERO_CONTA_INICIAL;
        salvar_dados_arquivo(banco);
        return;
    }

    int qtd = 0;
    int prox_num = NUMERO_CONTA_INICIAL;
    if (fscanf(fp, "%d %d\n", &qtd, &prox_num) != 2 || qtd < 0) {
        fclose(fp);
        banco->quantidade = 0;
        banco->proximo_numero = NUMERO_CONTA_INICIAL;
        printf("[AVISO] Arquivo '%s' vazio ou em formato antigo. Iniciando base limpa.\n", ARQUIVO_BANCO);
        salvar_dados_arquivo(banco);
        return;
    }

    banco->quantidade = 0;
    banco->proximo_numero = (prox_num >= NUMERO_CONTA_INICIAL) ? prox_num : NUMERO_CONTA_INICIAL;

    for (int i = 0; i < qtd; i++) {
        if (banco->quantidade == banco->capacidade) {
            if (!expandir_capacidade(banco)) {
                break;
            }
        }

        Carteira temp;
        memset(&temp, 0, sizeof(Carteira));

        if (fscanf(fp, " %99[^\n]\n", temp.cliente) != 1) {
            break;
        }

        if (fscanf(fp, "%d %d %d %d %lf %lf %lf %d %lf %d %d %lf %d %d\n",
                   &temp.numero, &temp.tem_corrente, &temp.tem_salario, &temp.tem_poupanca,
                   &temp.saldo_corrente, &temp.saldo_salario, &temp.saldo_poupanca,
                   &temp.emprestimo_realizado, &temp.valor_emprestimo,
                   &temp.possui_titulo, &temp.contemplado_sorteio,
                   &temp.valor_premio, &temp.conta_premio, &temp.qtd_historico) != 14) {
            break;
        }

        if (temp.qtd_historico < 0) temp.qtd_historico = 0;
        if (temp.qtd_historico > MAX_HISTORICO) temp.qtd_historico = MAX_HISTORICO;

        for (int j = 0; j < temp.qtd_historico; j++) {
            if (fscanf(fp, "%lf %c\n", &temp.historico[j].valor, &temp.historico[j].op) != 2) {
                temp.qtd_historico = j;
                break;
            }
            if (!isfinite(temp.historico[j].valor)) {
                temp.historico[j].valor = 0.0;
            }
        }

        // Sanitiza registros corrompidos ou que violem a regra Corrente + Salario
        if ((temp.tem_corrente && temp.tem_salario) ||
            !isfinite(temp.saldo_corrente) ||
            !isfinite(temp.saldo_salario) ||
            !isfinite(temp.saldo_poupanca) ||
            !isfinite(temp.valor_emprestimo) ||
            !isfinite(temp.valor_premio)) {
            continue;
        }

        temp.saldo_corrente = arredondar_centavos(temp.saldo_corrente);
        temp.saldo_salario = arredondar_centavos(temp.saldo_salario);
        temp.saldo_poupanca = arredondar_centavos(temp.saldo_poupanca);
        temp.valor_emprestimo = arredondar_centavos(temp.valor_emprestimo);
        temp.valor_premio = arredondar_centavos(temp.valor_premio);

        if (temp.numero >= banco->proximo_numero) {
            banco->proximo_numero = temp.numero + 1;
        }

        banco->clientes[banco->quantidade] = temp;
        banco->quantidade++;
    }

    fclose(fp);
}

void abrir_banco(Banco *banco) {
    if (banco->aberto == 1) {
        printf("\n[AVISO] O banco ja esta aberto!\n");
    } else {
        banco->aberto = 1;
        carregar_dados_arquivo(banco);
        printf("\n[SUCESSO] Banco aberto! Dados carregados (%d clientes).\n", banco->quantidade);
    }
}

void cadastrar_cliente(Banco *banco) {
    if (banco->aberto == 0) {
        printf("\n[ERRO] Abra o banco antes de cadastrar clientes.\n");
        return;
    }

    if (banco->proximo_numero > 9999) {
        printf("\n[ERRO] Limite maximo de numeros de conta (9999) atingido.\n");
        return;
    }

    if (banco->quantidade == banco->capacidade) {
        if (!expandir_capacidade(banco)) {
            return;
        }
    }

    Carteira novo;
    memset(&novo, 0, sizeof(Carteira));

    printf("\n--- Cadastro de Cliente ---\n");
    printf("Nome do cliente: ");
    if (!ler_linha(novo.cliente, sizeof(novo.cliente)) || novo.cliente[0] == '\0') {
        printf("[ERRO] Nome do cliente nao pode ser vazio.\n");
        return;
    }

    char codigo_tipo[32];
    printf("\nEscolha o tipo/combinacao de conta (Proibido Corrente + Salario):\n");
    printf("  01    -> Conta Corrente\n");
    printf("  02    -> Conta Salario\n");
    printf("  03    -> Conta Poupanca\n");
    printf("  01+03 -> Conta Corrente + Conta Poupanca\n");
    printf("  02+03 -> Conta Salario + Conta Poupanca\n");
    printf("Codigo: ");
    if (!ler_linha(codigo_tipo, sizeof(codigo_tipo)) || codigo_tipo[0] == '\0') {
        printf("[ERRO] Modalidade invalida!\n");
        return;
    }

    int tem_01 = 0, tem_02 = 0, tem_03 = 0;
    int duplicado = 0, invalido = 0;
    const char *p = codigo_tipo;

    while (*p != '\0') {
        const char *mais = strchr(p, '+');
        size_t len = (mais != NULL) ? (size_t)(mais - p) : strlen(p);

        if (len == 2 && p[0] == '0' && p[1] == '1') {
            if (tem_01) duplicado = 1;
            tem_01 = 1;
        } else if (len == 2 && p[0] == '0' && p[1] == '2') {
            if (tem_02) duplicado = 1;
            tem_02 = 1;
        } else if (len == 2 && p[0] == '0' && p[1] == '3') {
            if (tem_03) duplicado = 1;
            tem_03 = 1;
        } else {
            invalido = 1;
            break;
        }

        if (mais != NULL) {
            p = mais + 1;
            if (*p == '\0') {
                invalido = 1;
                break;
            }
        } else {
            break;
        }
    }

    if (invalido) {
        printf("[ERRO] Modalidade invalida! Use 01, 02, 03 ou combinacoes validas com '+'.\n");
        return;
    }
    if (duplicado) {
        printf("[ERRO] Nao e permitido cadastrar contas duplicadas!\n");
        return;
    }
    if (tem_01 && tem_02) {
        printf("[ERRO] O cliente NAO pode possuir Conta Corrente e Conta Salario simultaneamente!\n");
        return;
    }

    novo.tem_corrente = tem_01;
    novo.tem_salario = tem_02;
    novo.tem_poupanca = tem_03;

    // Todos os clientes sao cadastrados inicialmente com saldos zerados (0.00)
    novo.numero = banco->proximo_numero++;
    novo.saldo_corrente = 0.0;
    novo.saldo_salario = 0.0;
    novo.saldo_poupanca = 0.0;

    banco->clientes[banco->quantidade] = novo;
    banco->quantidade++;
    salvar_dados_arquivo(banco);

    printf("\n[SUCESSO] Cliente '%s' cadastrado com saldos zerados (R$ 0.00)!\n", novo.cliente);
    if (novo.tem_corrente) printf("  -> Conta Corrente: %04d-01\n", novo.numero);
    if (novo.tem_salario)  printf("  -> Conta Salario:  %04d-02\n", novo.numero);
    if (novo.tem_poupanca) printf("  -> Conta Poupanca: %04d-03\n", novo.numero);
}

void consultar_historico_banco(const Banco *banco) {
    if (banco->aberto == 0) {
        printf("\n[BLOQUEADO] O historico so pode ser consultado com o banco aberto!\n");
        return;
    }

    double saldo_total_banco = 0.0;
    for (int i = 0; i < banco->quantidade; i++) {
        saldo_total_banco += calcular_saldo_total_cliente(&banco->clientes[i]);
    }
    saldo_total_banco = arredondar_centavos(saldo_total_banco);

    printf("\n== Saldo Total do banco ==\n\n");
    printf("%d | R$ %.2f\n", banco->quantidade, saldo_total_banco);

    printf("\n== Clientes Aptos a Emprestimos ==\n\n");
    int encontrou = 0;
    for (int i = 0; i < banco->quantidade; i++) {
        const Carteira *c = &banco->clientes[i];
        double saldo_total = calcular_saldo_total_cliente(c);
        if (c->emprestimo_realizado == 0 &&
            c->tem_corrente == 1 &&
            c->tem_poupanca == 1 &&
            c->saldo_poupanca > 0.0 &&
            saldo_total > 0.0) {
            double limite = arredondar_centavos(0.25 * (10.0 * saldo_total));
            printf("%s|%04d-01|R$ %.2f\n", c->cliente, c->numero, limite);
            encontrou = 1;
        }
    }
    if (!encontrou) printf("(Nenhum)\n");

    printf("\n== Clientes com Saldo Negativo na Conta Salario ==\n\n");
    encontrou = 0;
    for (int i = 0; i < banco->quantidade; i++) {
        const Carteira *c = &banco->clientes[i];
        if (c->tem_corrente == 1 && c->saldo_corrente < 0.0) {
            printf("%s|%04d-01|R$ %.2f\n", c->cliente, c->numero, c->saldo_corrente);
            encontrou = 1;
        }
        if (c->tem_salario == 1 && c->saldo_salario < 0.0) {
            printf("%s|%04d-02|R$ %.2f\n", c->cliente, c->numero, c->saldo_salario);
            encontrou = 1;
        }
    }
    if (!encontrou) printf("(Nenhum)\n");

    printf("\n== Clientes Aptos a Compra de Titulos ==\n\n");
    encontrou = 0;
    for (int i = 0; i < banco->quantidade; i++) {
        const Carteira *c = &banco->clientes[i];
        if (c->tem_salario == 1 && c->saldo_salario >= 1000.0 && c->saldo_salario <= 1500.0) {
            printf("%s|%04d-02|R$ %.2f\n", c->cliente, c->numero, c->saldo_salario);
            encontrou = 1;
        }
    }
    if (!encontrou) printf("(Nenhum)\n");

    printf("\n== Clientes Contemplado no Sorteio ==\n\n");
    encontrou = 0;
    for (int i = 0; i < banco->quantidade; i++) {
        const Carteira *c = &banco->clientes[i];
        if (c->contemplado_sorteio == 1) {
            int tipo = (c->conta_premio >= 1 && c->conta_premio <= 3) ? c->conta_premio : 2;
            printf("%s|%04d-%02d|R$ %.2f\n", c->cliente, c->numero, tipo, c->valor_premio);
            encontrou = 1;
        }
    }
    if (!encontrou) printf("(Nenhum)\n");
}

void efetuar_sorteio(Banco *banco) {
    if (banco->aberto == 0) {
        printf("\n[BLOQUEADO] O sorteio so pode ser efetuado com o banco aberto!\n");
        return;
    }

    if (banco->quantidade < QTD_FINALISTAS_SORTEIO) {
        printf("\n[AVISO] E necessario ter pelo menos %d clientes cadastrados para realizar o sorteio.\n",
               QTD_FINALISTAS_SORTEIO);
        return;
    }

    // Aloca dinamicamente o vetor de indices dos clientes elegiveis (com titulo e saldo positivo)
    int *elegiveis = (int *)malloc((size_t)banco->quantidade * sizeof(int));
    if (elegiveis == NULL) {
        printf("[ERRO] Falha ao alocar memoria para o sorteio.\n");
        return;
    }

    int qtd_elegiveis = 0;
    for (int i = 0; i < banco->quantidade; i++) {
        const Carteira *c = &banco->clientes[i];
        int tem_saldo_positivo = (c->saldo_salario > 0.0) ||
                                 (c->tem_poupanca == 1 && c->saldo_poupanca > 0.0);
        if (c->possui_titulo == 1 && tem_saldo_positivo) {
            elegiveis[qtd_elegiveis++] = i;
        }
    }

    if (qtd_elegiveis < QTD_FINALISTAS_SORTEIO) {
        printf("\n[BLOQUEADO] Sao necessarios pelo menos %d clientes com titulos de capitalizacao e saldo positivo para o sorteio (atual: %d).\n",
               QTD_FINALISTAS_SORTEIO, qtd_elegiveis);
        free(elegiveis);
        return;
    }

    // Ordena os clientes elegiveis em ordem decrescente de saldo total
    for (int i = 0; i < qtd_elegiveis - 1; i++) {
        int max_idx = i;
        double max_saldo = calcular_saldo_total_cliente(&banco->clientes[elegiveis[i]]);
        for (int j = i + 1; j < qtd_elegiveis; j++) {
            double saldo_j = calcular_saldo_total_cliente(&banco->clientes[elegiveis[j]]);
            if (saldo_j > max_saldo) {
                max_saldo = saldo_j;
                max_idx = j;
            }
        }
        if (max_idx != i) {
            int tmp = elegiveis[i];
            elegiveis[i] = elegiveis[max_idx];
            elegiveis[max_idx] = tmp;
        }
    }

    // Pesos de Fibonacci para os 5 finalistas: 5, 3, 2, 1, 1 (soma = 12)
    int pesos[QTD_FINALISTAS_SORTEIO];
    int soma_pesos = 0;
    for (int i = 0; i < QTD_FINALISTAS_SORTEIO; i++) {
        pesos[i] = fibonacci(QTD_FINALISTAS_SORTEIO - i);
        soma_pesos += pesos[i];
    }

    printf("\n=== SORTEIO FIBONACCI (TOP %d COM TITULOS DE CAPITALIZACAO) ===\n", QTD_FINALISTAS_SORTEIO);
    for (int i = 0; i < QTD_FINALISTAS_SORTEIO; i++) {
        Carteira *c = &banco->clientes[elegiveis[i]];
        printf("  %d lugar: %s (%04d-02) | Saldo Total: R$ %.2f | Peso Fibonacci: %d/%d\n",
               i + 1, c->cliente, c->numero, calcular_saldo_total_cliente(c), pesos[i], soma_pesos);
    }

    int sorteado = rand() % soma_pesos;
    int acumulado = 0;
    int indice_vencedor = elegiveis[0];
    for (int i = 0; i < QTD_FINALISTAS_SORTEIO; i++) {
        acumulado += pesos[i];
        if (sorteado < acumulado) {
            indice_vencedor = elegiveis[i];
            break;
        }
    }

    Carteira *vencedor = &banco->clientes[indice_vencedor];

    // Define a conta contemplada (prioriza a conta de maior saldo disponivel)
    int tipo_premio = 2; // Conta Salario (todo cliente com titulo possui conta salario)
    double saldo_conta = vencedor->saldo_salario;
    if (vencedor->tem_poupanca == 1 && vencedor->saldo_poupanca > vencedor->saldo_salario) {
        tipo_premio = 3;
        saldo_conta = vencedor->saldo_poupanca;
    }

    double premio = arredondar_centavos(10.0 * saldo_conta);

    if (tipo_premio == 3) {
        vencedor->saldo_poupanca = arredondar_centavos(vencedor->saldo_poupanca + premio);
    } else {
        vencedor->saldo_salario = arredondar_centavos(vencedor->saldo_salario + premio);
    }

    vencedor->contemplado_sorteio = 1;
    vencedor->valor_premio = arredondar_centavos(vencedor->valor_premio + premio);
    vencedor->conta_premio = tipo_premio;

    salvar_dados_arquivo(banco);

    printf("\n[CONTEMPLADO] Cliente vencedor: %s (%04d-%02d)\n",
           vencedor->cliente, vencedor->numero, tipo_premio);
    printf("Premio (10x saldo da conta): R$ %.2f\n", premio);
    printf("Novo saldo na conta %04d-%02d: R$ %.2f\n",
           vencedor->numero, tipo_premio, obter_saldo_conta(vencedor, tipo_premio));

    free(elegiveis);
}

static void exibir_alerta_saldo_negativo(const Carteira *c) {
    if (c->tem_corrente == 1 && c->saldo_corrente < 0.0) {
        printf("\n[ALERTA] Sua Conta Corrente (%04d-01) esta com saldo NEGATIVO: R$ %.2f!\n",
               c->numero, c->saldo_corrente);
    }
    if (c->tem_salario == 1 && c->saldo_salario < 0.0) {
        printf("\n[ALERTA] Sua Conta Salario (%04d-02) esta com saldo NEGATIVO: R$ %.2f!\n",
               c->numero, c->saldo_salario);
    }
}

void acessar_conta_cliente(Banco *banco) {
    if (banco->aberto == 0) {
        printf("\n[ERRO] O banco precisa estar aberto para acessar contas.\n");
        return;
    }

    char entrada[32];
    printf("\nDigite o numero da conta (formato NNNN-TT, ex: 1000-01): ");
    if (!ler_linha(entrada, sizeof(entrada))) {
        return;
    }

    // Valida estritamente 4 digitos (1000-9999) + '-' + codigo de 2 digitos (01, 02 ou 03)
    if (strlen(entrada) != 7 ||
        !isdigit((unsigned char)entrada[0]) ||
        !isdigit((unsigned char)entrada[1]) ||
        !isdigit((unsigned char)entrada[2]) ||
        !isdigit((unsigned char)entrada[3]) ||
        entrada[0] == '0' ||
        entrada[4] != '-' ||
        entrada[5] != '0' ||
        (entrada[6] != '1' && entrada[6] != '2' && entrada[6] != '3')) {
        printf("[ERRO] Formato invalido! Use 4 digitos (1000-9999) seguidos de '-01', '-02' ou '-03'. Ex: 1000-01.\n");
        return;
    }

    int num_conta = (entrada[0] - '0') * 1000 +
                    (entrada[1] - '0') * 100 +
                    (entrada[2] - '0') * 10 +
                    (entrada[3] - '0');
    int tipo_conta = entrada[6] - '0';

    for (int i = 0; i < banco->quantidade; i++) {
        Carteira *c = &banco->clientes[i];
        if (c->numero == num_conta) {
            int possui_tipo = (tipo_conta == 1 && c->tem_corrente == 1) ||
                              (tipo_conta == 2 && c->tem_salario == 1) ||
                              (tipo_conta == 3 && c->tem_poupanca == 1);
            if (!possui_tipo) {
                printf("[ERRO] O cliente %s (%04d) nao possui Conta %s (%02d).\n",
                       c->cliente, c->numero, nome_tipo_conta(tipo_conta), tipo_conta);
                return;
            }

            menu_cliente(banco, c, tipo_conta);
            return;
        }
    }

    printf("[ERRO] Conta %04d-%02d nao encontrada.\n", num_conta, tipo_conta);
}

void menu_cliente(Banco *banco, Carteira *cliente, int tipo_conta) {
    int opcao = -1;

    do {
        exibir_alerta_saldo_negativo(cliente);

        printf("\n=== VISAO DO CLIENTE ===\n");
        printf("Cliente: %s | Conta: %04d-%02d (%s)\n",
               cliente->cliente, cliente->numero, tipo_conta, nome_tipo_conta(tipo_conta));
        printf("Saldo da conta: R$ %.2f | Saldo total: R$ %.2f\n",
               obter_saldo_conta(cliente, tipo_conta), calcular_saldo_total_cliente(cliente));
        printf("[1] Depositar\n");
        printf("[2] Sacar\n");
        printf("[3] Solicitar emprestimo\n");
        printf("[4] Adquirir titulos de capitalizacao\n");
        printf("[5] Solicitar extrato\n");
        printf("[0] Voltar ao menu principal\n");
        printf("Opcao: ");

        int status = ler_inteiro(&opcao);
        if (status == -1) {
            printf("\nFim da entrada detectado. Saindo da conta %04d-%02d...\n",
                   cliente->numero, tipo_conta);
            break;
        }
        if (status == 0) {
            opcao = -1;
            printf("\n[ERRO] Opcao invalida!\n");
            continue;
        }

        switch (opcao) {
            case 1:
                realizar_deposito(banco, cliente, tipo_conta);
                break;
            case 2:
                realizar_saque(banco, cliente, tipo_conta);
                break;
            case 3:
                solicitar_emprestimo(banco, cliente);
                break;
            case 4:
                adquirir_titulo_capitalizacao(banco, cliente);
                break;
            case 5:
                solicitar_extrato(cliente);
                break;
            case 0:
                printf("\nSaindo da conta %04d-%02d...\n", cliente->numero, tipo_conta);
                break;
            default:
                printf("\n[ERRO] Opcao invalida!\n");
                break;
        }
    } while (opcao != 0);
}

void realizar_deposito(Banco *banco, Carteira *cliente, int tipo_conta) {
    double valor = 0.0;
    printf("\nValor do deposito na conta %04d-%02d: R$ ", cliente->numero, tipo_conta);

    int status = ler_valor_monetario(&valor);
    if (status == -1) {
        return;
    }
    if (status == 0) {
        printf("[ERRO] Valor invalido.\n");
        return;
    }

    if (valor <= 0.0) {
        printf("[ERRO] O valor do deposito deve ser maior que zero.\n");
        return;
    }

    if (tipo_conta == 1) {
        cliente->saldo_corrente = arredondar_centavos(cliente->saldo_corrente + valor);
    } else if (tipo_conta == 2) {
        cliente->saldo_salario = arredondar_centavos(cliente->saldo_salario + valor);
    } else if (tipo_conta == 3) {
        cliente->saldo_poupanca = arredondar_centavos(cliente->saldo_poupanca + valor);
    }

    registrar_transacao(cliente, 'D', valor);
    salvar_dados_arquivo(banco);
    printf("[SUCESSO] Deposito de R$ %.2f realizado! Novo saldo: R$ %.2f\n",
           valor, obter_saldo_conta(cliente, tipo_conta));
}

void realizar_saque(Banco *banco, Carteira *cliente, int tipo_conta) {
    double valor = 0.0;
    printf("\nValor do saque da conta %04d-%02d: R$ ", cliente->numero, tipo_conta);

    int status = ler_valor_monetario(&valor);
    if (status == -1) {
        return;
    }
    if (status == 0) {
        printf("[ERRO] Valor invalido.\n");
        return;
    }

    if (valor <= 0.0) {
        printf("[ERRO] O valor do saque deve ser maior que zero.\n");
        return;
    }

    // Conta Poupanca nao permite saldo negativo; Corrente e Salario permitem
    if (tipo_conta == 3 && valor > arredondar_centavos(cliente->saldo_poupanca) + 1e-9) {
        printf("[NEGADO] Saldo insuficiente na Conta Poupanca (disponivel: R$ %.2f).\n",
               cliente->saldo_poupanca);
        return;
    }

    if (tipo_conta == 1) {
        cliente->saldo_corrente = arredondar_centavos(cliente->saldo_corrente - valor);
    } else if (tipo_conta == 2) {
        cliente->saldo_salario = arredondar_centavos(cliente->saldo_salario - valor);
    } else if (tipo_conta == 3) {
        cliente->saldo_poupanca = arredondar_centavos(cliente->saldo_poupanca - valor);
    }

    registrar_transacao(cliente, 'S', valor);
    salvar_dados_arquivo(banco);
    printf("[SUCESSO] Saque de R$ %.2f realizado! Novo saldo: R$ %.2f\n",
           valor, obter_saldo_conta(cliente, tipo_conta));
}

void solicitar_emprestimo(Banco *banco, Carteira *cliente) {
    if (cliente->emprestimo_realizado == 1) {
        printf("\n[NEGADO] Apenas um emprestimo e permitido por cliente (ja realizado: R$ %.2f).\n",
               cliente->valor_emprestimo);
        return;
    }

    if (cliente->tem_corrente != 1 || cliente->tem_poupanca != 1) {
        printf("\n[NEGADO] E necessario possuir Conta Corrente e Conta Poupanca para solicitar emprestimo.\n");
        return;
    }

    if (cliente->saldo_poupanca <= 0.0) {
        printf("\n[NEGADO] E necessario possuir saldo positivo na Conta Poupanca.\n");
        return;
    }

    double saldo_total = calcular_saldo_total_cliente(cliente);
    if (saldo_total <= 0.0) {
        printf("\n[NEGADO] O saldo total da carteira deve ser positivo para calcular o limite.\n");
        return;
    }

    double limite = arredondar_centavos(0.25 * (10.0 * saldo_total));
    printf("\nCliente apto! Limite maximo de emprestimo (25%% de 10x o saldo total): R$ %.2f\n", limite);
    printf("Valor do emprestimo desejado: R$ ");

    double valor = 0.0;
    int status = ler_valor_monetario(&valor);
    if (status == -1) {
        return;
    }
    if (status == 0) {
        printf("[ERRO] Valor invalido.\n");
        return;
    }

    if (valor <= 0.0 || valor > limite + 1e-9) {
        printf("[NEGADO] Valor invalido ou acima do limite permitido (R$ %.2f).\n", limite);
        return;
    }

    cliente->valor_emprestimo = valor;
    cliente->saldo_corrente = arredondar_centavos(cliente->saldo_corrente + valor);
    cliente->emprestimo_realizado = 1;

    registrar_transacao(cliente, 'E', valor);
    salvar_dados_arquivo(banco);

    printf("[SUCESSO] Emprestimo de R$ %.2f creditado na Conta Corrente (%04d-01)!\n",
           valor, cliente->numero);
}

void adquirir_titulo_capitalizacao(Banco *banco, Carteira *cliente) {
    if (cliente->tem_salario != 1) {
        printf("\n[NEGADO] Apenas clientes com Conta Salario podem adquirir titulos de capitalizacao.\n");
        return;
    }

    if (cliente->saldo_salario < 1000.0 || cliente->saldo_salario > 1500.0) {
        printf("\n[NEGADO] E necessario ter entre R$ 1000.00 e R$ 1500.00 na Conta Salario (saldo atual: R$ %.2f).\n",
               cliente->saldo_salario);
        return;
    }

    if (cliente->possui_titulo == 1) {
        printf("\n[AVISO] Voce ja possui titulo de capitalizacao ativo e participa dos sorteios!\n");
        return;
    }

    cliente->possui_titulo = 1;
    salvar_dados_arquivo(banco);
    printf("\n[SUCESSO] Titulo de capitalizacao adquirido! Voce agora esta apto a participar do sorteio.\n");
}

void solicitar_extrato(const Carteira *cliente) {
    printf("\n--- EXTRATO (Ultimas %d transacoes) ---\n", cliente->qtd_historico);
    if (cliente->qtd_historico == 0) {
        printf("Nenhuma transacao registrada.\n");
        return;
    }

    for (int i = 0; i < cliente->qtd_historico; i++) {
        printf("%.2f :%c\n", cliente->historico[i].valor, cliente->historico[i].op);
    }
}
