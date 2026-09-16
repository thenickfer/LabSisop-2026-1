#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h> 

int checknum(const char *str) {
    if (str == NULL || *str == '\0') return 0;
    while (*str) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

int listaprincipal(){
    FILE *arquivo = fopen("/proc/version", "r");
    char linha[256];

    printf("<h3>Informações gerais do sistema:</h3><p>Versão: %s</p>", fgets(linha, sizeof(linha), arquivo));
    arquivo = fopen("/proc/uptime", "r");
    double uptime_sec;
    double idletime_sec;

    if (fscanf(arquivo, "%lf %lf", &uptime_sec, &idletime_sec) != 2) {
        fprintf(stderr, "Erro ao ler os dados do uptime.\n");
        fclose(arquivo);
        return 1;
    }
    long total_su = (long)uptime_sec;
    long total_si = (long)idletime_sec;

    printf("<p>Uptime: %d dias, %d horas, %d minutos e %d segundos</p>", total_su / 86400, total_su % 86400 / 3600, total_su % 86400 % 3600 / 60, total_su % 86400 % 3600 % 60);
    printf("<p>Idle time: %d dias, %d horas, %d minutos e %d segundos</p>", total_si / 86400, total_si % 86400 / 3600, total_si % 86400 % 3600 / 60, total_si % 86400 % 3600 % 60);

    time_t tempo_atual;
    struct tm *info_tempo;
    char buffer[80];

    time(&tempo_atual);
    info_tempo = localtime(&tempo_atual);
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", info_tempo);

    printf("<p>Data/hora do sistema: %s</p>", buffer);

    arquivo = fopen("/proc/cpuinfo", "r");
    char modelo[128] = "Desconhecido";
    char velocidade[64] = "Desconhecida";
    int nucleos = 0;

    while (fgets(linha, sizeof(linha), arquivo)) {
        if (strncmp(linha, "model name", 10) == 0) {
            char *ptr = strchr(linha, ':');
            if (ptr) strcpy(modelo, ptr + 2);
            modelo[strcspn(modelo, "\n")] = 0;
        } else if (strncmp(linha, "cpu MHz", 7) == 0) {
            char *ptr = strchr(linha, ':');
            if (ptr) strcpy(velocidade, ptr + 2);
            velocidade[strcspn(velocidade, "\n")] = 0;
        } else if (strncmp(linha, "processor", 9) == 0) {
            nucleos++; // Contando threads lógicas
        }
    }

    printf("<p>Modelo: %s <BR>", modelo);
    printf("Velocidade Atual: %s MHz <BR>", velocidade);
    printf("Total de Núcleos (Lógicos): %d</p>", nucleos);

    arquivo = fopen("/proc/loadavg", "r");
    double lavg1, lavg5, lavg15;
    if (fscanf(arquivo, "%lf %lf %lf", &lavg1, &lavg5, &lavg15) == 3) {
        printf("<p>Carga do sistema: <BR>");
        printf("Média (1 min): %.2f | (5 min): %.2f | (15 min): %.2f</p>", lavg1, lavg5, lavg15);
    }

    long long user1, nice1, system1, idle1, iowait1, irq1, softirq1, steal1;
    long long user2, nice2, system2, idle2, iowait2, irq2, softirq2, steal2;

    arquivo = fopen("/proc/stat", "r");
    if (!arquivo) {
        perror("Erro ao abrir /proc/stat na amostra 1");
    }

    fscanf(arquivo, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", 
           &user1, &nice1, &system1, &idle1, &iowait1, &irq1, &softirq1, &steal1);
    fclose(arquivo);

    long long total_1 = user1 + nice1 + system1 + idle1 + iowait1 + irq1 + softirq1 + steal1;
    long long ocupado_1 = total_1 - idle1 - iowait1;

    sleep(1); 

    arquivo = fopen("/proc/stat", "r");
    if (arquivo) {
        perror("Erro ao abrir /proc/stat na amostra 2");
    }
    fscanf(arquivo, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", 
           &user2, &nice2, &system2, &idle2, &iowait2, &irq2, &softirq2, &steal2);
    fclose(arquivo);

    // Cálculos da Amostra 2
    long long total_2 = user2 + nice2 + system2 + idle2 + iowait2 + irq2 + softirq2 + steal2;
    long long ocupado_2 = total_2 - idle2 - iowait2;


    // --- CÁLCULO DA OCUPAÇÃO EM PERCENTUAL ---
    long long delta_total = total_2 - total_1;
    
    printf("<p>Ocupação: <BR>");
    if (delta_total > 0) {
        double ocupacao_pct = 100.0 * (ocupado_2 - ocupado_1) / delta_total;
        printf("Ocupação do processador: %.2f%%</p>", ocupacao_pct);
    } else {
        printf("Ocupação do processador: 0.00%% (Amostra estática)</p>");
    }

    arquivo = fopen("/proc/meminfo", "r");
    char chave[64];
    long valor;
    long total = 0, livre = 0, buffers = 0, cached = 0;

    while (fscanf(arquivo, "%s %ld kB", chave, &valor) != EOF) {
        if (strcmp(chave, "MemTotal:") == 0) total = valor;
        else if (strcmp(chave, "MemFree:") == 0) livre = valor;
        else if (strcmp(chave, "Buffers:") == 0) buffers = valor;
        else if (strcmp(chave, "Cached:") == 0) cached = valor;
    }

    long usada = total - livre - buffers - cached;
    printf("<p>Memória: <BR>\n");
    printf("Total: %ld MB <BR>", total / 1024);
    printf("Usada: %ld MB</p>", usada / 1024);

    arquivo = fopen("/proc/diskstats", "r");
    unsigned int maior, menor;
    char nome_disco[32];
    unsigned long leituras, escritas, ler_setores, escrever_setores;
    
    printf("<p> Disco");
    while (fgets(nome_disco, sizeof(nome_disco), arquivo)) {
        char linha[256];
        if (sscanf(nome_disco, "%u %u %s %lu %*u %*u %*u %lu", &maior, &menor, linha, &leituras, &escritas) == 5) {
            if (leituras > 0 || escritas > 0) {
                printf("<BR>Dispositivo [%s] -> Leituras completadas: %lu | Escritas completadas: %lu\n", linha, leituras, escritas);
            }
        }
    }
    printf("</p>");

    arquivo = fopen("/proc/filesystems", "r");
    char tipo[32], nome[32];
    printf("<p>Sistemas de arquivos: <BR>");
    while (fscanf(arquivo, "%s", tipo) != EOF) {
        if (strcmp(tipo, "nodev") == 0) {
            fscanf(arquivo, "%s", nome);
            printf("| %s (virtual) <BR>", nome);
        } else {
            printf("| %s (nativo/bloco) <BR>", tipo);
        }
    }
    printf("</p>");

    arquivo = fopen("/proc/devices", "r");
    linha[128];
    printf("<p>Dispositivos: <BR>");
    while (fgets(linha, sizeof(linha), arquivo)) {
        printf("%s<BR>", linha);
    }
    printf("</p>");

    arquivo = fopen("/proc/net/dev", "r");
    linha[256];
    
    printf("<p> DISPOSITIVOS DE REDE <BR>\n");
    fgets(linha, sizeof(linha), arquivo);
    fgets(linha, sizeof(linha), arquivo);
    
    while (fgets(linha, sizeof(linha), arquivo)) {
        char *interface = strtok(linha, ":");
        if (interface) {
            while(isspace((unsigned char)*interface)) interface++;
            
            printf("Interface de Rede ativa encontrada: %s</p>\n", interface);
        }
    }

    DIR *dir = opendir("/proc");
    struct dirent *entrada;

    printf("<h2>Processos</h2>\n");

    while ((entrada = readdir(dir)) != NULL) {
        int eh_pid = 1;
        for (int i = 0; entrada->d_name[i] != '\0'; i++) {
            if (!isdigit((unsigned char)entrada->d_name[i])) {
                eh_pid = 0;
                break;
            }
        }

        if (eh_pid) {
            char caminho_comm[256];
            snprintf(caminho_comm, sizeof(caminho_comm), "/proc/%s/comm", entrada->d_name);
            
            arquivo = fopen(caminho_comm, "r");
            if (arquivo) {
                char nome_processo[128];
                if (fgets(nome_processo, sizeof(nome_processo), arquivo)) {
                    nome_processo[strcspn(nome_processo, "\n")] = 0; 

                    printf("<p>[%s] %-20s -> <a href=\"/cgi-bin/monitor?pid=%s\">Ver Detalhes</a></p>\n", 
                           entrada->d_name, nome_processo, entrada->d_name);
                }
        
            }
        }
    }
    closedir(dir);
}

int detalhaProc(char *pid){
    if (pid[0] == '\0' || !checknum(pid)) {
        printf("<p style='color:red;'>Erro: PID inválido foi informado na URL.</p>");
        printf("<br><a href='/index.html'>&larr; Voltar para a lista</a>");
        printf("</body></html>");
        return 0;
    } 

    char caminho[64];
    snprintf(caminho, sizeof(caminho), "/proc/%s/status", pid);

    FILE *arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        printf("<p style='color:red;'>Erro: O processo %s não existe ou já foi finalizado.</p>", pid);
    } else {
        printf("<h3>Informações do PID: %s</h3>", pid);
        printf("<pre style='background:#f4f4f4; padding:10px; font-family: monospace;'>");
        
        char linha[256];
        while (fgets(linha, sizeof(linha), arquivo)) {
            if (strncmp(linha, "Name:", 5) == 0) {
                printf("%s", linha);
            } 
            else if (strncmp(linha, "State:", 6) == 0) {
                printf("%s", linha);
            } 
            else if (strncmp(linha, "PPid:", 5) == 0) {
                printf("%s", linha);
            } 
            else if (strncmp(linha, "Uid:", 4) == 0) {
                printf("%s", linha);
            } 
            else if (strncmp(linha, "Threads:", 8) == 0) {
                printf("%s", linha);
            } 
            else if (strncmp(linha, "VmSize:", 7) == 0) {
                printf("%s", linha);
            } 
            else if (strncmp(linha, "VmRSS:", 6) == 0) {
                printf("%s", linha);
            }
        }
        fclose(arquivo);

        snprintf(caminho, sizeof(caminho), "/proc/%s/stat", pid);
        arquivo = fopen(caminho, "r");

        if (arquivo != NULL) {
            int dummy_pid;
            fscanf(arquivo, "%d %*[^)]) ", &dummy_pid);
            fscanf(arquivo, "%*c "); 
            
            for (int i = 4; i <= 13; i++) {
                fscanf(arquivo, "%*ld ");
            }

            long utime = 0;
            long stime = 0;
            long priority = 0;
            long nice = 0;

            if (fscanf(arquivo, "%ld %ld %*ld %*ld %ld %ld", &utime, &stime, &priority, &nice) == 4) {
                long ticks_por_segundo = sysconf(_SC_CLK_TCK);
                double utime_segundos = (double)utime / ticks_por_segundo;
                double stime_segundos = (double)stime / ticks_por_segundo;
                
                printf("Tempo Usuário (utime): %.2f segundos\n", utime_segundos);
                printf("Tempo Sistema (stime): %.2f segundos\n", stime_segundos);
                printf("Prioridade (priority): %ld\n", priority);
                printf("Valor Nice (nice):     %ld\n", nice);
            } else {
                printf("Erro ao ler os campos do stat\n");
            }
            fclose(arquivo);
        }
        printf("</pre>");

        snprintf(caminho, sizeof(caminho), "/proc/%s/cmdline", pid);
        arquivo = fopen(caminho, "rb");

        printf("<p><strong>Linha de comando:</strong></p>");
        if (arquivo != NULL) {
            char buffer[4096];
            size_t bytes_lidos = fread(buffer, 1, sizeof(buffer) - 1, arquivo);
            fclose(arquivo);

            if (bytes_lidos == 0) {
                printf("<p><em>[Processo de Kernel]</em></p>");
            } else {
                buffer[bytes_lidos] = '\0';
                printf("<pre style='background:#f4f4f4; padding:10px; font-family: monospace;'>");
                
                for (size_t i = 0; i < bytes_lidos; i++) {
                    if (buffer[i] == '\0') {
                        if (i + 1 < bytes_lidos && buffer[i + 1] == '\0') break;
                        if (i < bytes_lidos - 1) {
                            printf(" ");
                        }
                    } else {
                        printf("%c", buffer[i]);
                    }
                }
                printf("</pre>");
            }
        } else {
            printf("<p style='color:red;'>Erro ao abrir cmdline.</p>");
        }
    } 

    printf("<br><a href='/index.html'>&larr; Voltar para a lista</a>");
   
}
int main(void) {
    printf("Content-Type: text/html; charset=utf-8\n\n");
    printf("<!DOCTYPE html><html lang=\"pt-BR\"><head><title>Processos</title><meta http-equiv=\"refresh\" content=\"5\"></head><body>");
    char *query = getenv("QUERY_STRING");
    char pid[32] = {0};

    if (query != NULL) {
        char *pos = strstr(query, "pid=");
        if (pos != NULL) {
            pos += 4;
            int i = 0;
            while (*pos && *pos != '&' && i < 31) {
                pid[i++] = *pos++;
            }
            pid[i] = '\0';
        }
    }
    
    if(pid[0] != '\0'){
        detalhaProc(pid);
    } else {
        listaprincipal();
    }

    printf("</body></html>");
    return 0;
}
