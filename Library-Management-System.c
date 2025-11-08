#include <stdio.h>// library.c
// Windows console program (uses conio.h, windows.h).
// Compile with: gcc library.c -o library.exe  (or build in Visual Studio)

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>

#define MAX_BOOKS 100
#define MAX_STUDENTS 100
#define STR_MAX 128
#define LINE_BUF 512

// -------------------- Color & Arrow UI --------------------
void Color(int couleurDuTexte,int couleurDeFond)
{
    HANDLE H=GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(H,couleurDeFond*16+couleurDuTexte);
}

void arrowHere(int realposition,int arrowposition){
    if (realposition == arrowposition){
        Color(0,7); // selected: black text (0), white background (7)
    } else {
        Color(7,0); // normal: white text (7), black background (0)
    }
}

// Read arrow / enter keys robustly.
// Returns: 72 = up, 80 = down, 13 = enter, other ascii for other keys.
int read_arrow() {
    int c = getch();
    if (c == 0 || c == 224) {
        // extended key - arrow keys come here
        c = getch();
        return c; // e.g., 72 up, 80 down, 75 left, 77 right
    }
    return c; // e.g., 13 enter
}

// -------------------- Utilities --------------------
void clear_input() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}
}

void read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) {
        buf[0] = '\0';
        return;
    }
    size_t ln = strlen(buf);
    if (ln > 0 && buf[ln - 1] == '\n') buf[ln - 1] = '\0';
}

bool file_exists(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f) { fclose(f); return true; }
    return false;
}

// trim helpers
void trim_newline(char *s) {
    size_t ln = strlen(s);
    if (ln>0 && s[ln-1]=='\n') s[ln-1]='\0';
    if (ln>1 && s[ln-2]=='\r') s[ln-2]='\0';
}

// -------------------- Data Structures --------------------
struct library {
    char bookName[STR_MAX];
    char author[STR_MAX];
    bool d;
    int ref;
    int pages;
};

struct etudiant {
    char Name[STR_MAX];
    char pass[STR_MAX];
    int pan; // reference in library (0 if empty)
    int id;
};

// -------------------- Persistence (text files) --------------------
const char *BOOKS_FILE = "books.txt";
const char *STUDENTS_FILE = "students.txt";

bool save_books_txt(struct library *books, int count) {
    FILE *f = fopen(BOOKS_FILE, "w");
    if (!f) return false;
    // one book per line: ref|bookName|author|pages
    for (int i = 0; i < count; ++i) {
        // escape '|' not handled (keep simple), don't include newline in fields
        fprintf(f, "%d|%s|%s|%d\n", books[i].ref, books[i].bookName, books[i].author, books[i].pages);
    }
    fclose(f);
    return true;
}

int load_books_txt(struct library *books, int max_count) {
    if (!file_exists(BOOKS_FILE)) return 0;
    FILE *f = fopen(BOOKS_FILE, "r");
    if (!f) return 0;
    char line[LINE_BUF];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < max_count) {
        trim_newline(line);
        // parse: ref|bookName|author|pages
        char *p = strchr(line, '|');
        if (!p) continue;
        *p = '\0';
        int ref = atoi(line);
        char *name = p + 1;
        char *p2 = strchr(name, '|');
        if (!p2) continue;
        *p2 = '\0';
        char *author = p2 + 1;
        char *p3 = strchr(author, '|');
        if (!p3) continue;
        *p3 = '\0';
        char *pages_s = p3 + 1;
        int pages = atoi(pages_s);

        books[count].ref = ref; // we'll reassign refs later to be sure
        strncpy(books[count].bookName, name, STR_MAX-1); books[count].bookName[STR_MAX-1] = '\0';
        strncpy(books[count].author, author, STR_MAX-1); books[count].author[STR_MAX-1] = '\0';
        books[count].pages = pages;
        books[count].d = true;
        ++count;
    }
    fclose(f);
    // reassign refs contiguous
    for (int i=0;i<count;i++) books[i].ref = i+1;
    return count;
}

bool save_students_txt(struct etudiant *students, int count) {
    FILE *f = fopen(STUDENTS_FILE, "w");
    if (!f) return false;
    // one student per line: id|Name|pass|pan
    for (int i = 0; i < count; ++i) {
        fprintf(f, "%d|%s|%s|%d\n", students[i].id, students[i].Name, students[i].pass, students[i].pan);
    }
    fclose(f);
    return true;
}

int load_students_txt(struct etudiant *students, int max_count) {
    if (!file_exists(STUDENTS_FILE)) return 0;
    FILE *f = fopen(STUDENTS_FILE, "r");
    if (!f) return 0;
    char line[LINE_BUF];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < max_count) {
        trim_newline(line);
        // parse: id|Name|pass|pan
        char *p = strchr(line, '|');
        if (!p) continue;
        *p = '\0';
        int id = atoi(line);
        char *name = p + 1;
        char *p2 = strchr(name, '|');
        if (!p2) continue;
        *p2 = '\0';
        char *pass = p2 + 1;
        char *p3 = strchr(pass, '|');
        if (!p3) continue;
        *p3 = '\0';
        char *pan_s = p3 + 1;
        int pan = atoi(pan_s);

        students[count].id = id; // we'll reassign ids later
        strncpy(students[count].Name, name, STR_MAX-1); students[count].Name[STR_MAX-1] = '\0';
        strncpy(students[count].pass, pass, STR_MAX-1); students[count].pass[STR_MAX-1] = '\0';
        students[count].pan = pan;
        ++count;
    }
    fclose(f);
    for (int i=0;i<count;i++) students[i].id = i+1;
    return count;
}

// -------------------- Admin password handling --------------------
const char *ADMIN_PASS_FILE = "admin_pass.txt";

bool admin_password_exists() {
    return file_exists(ADMIN_PASS_FILE);
}

bool create_admin_password() {
    system("cls");
    Color(15,4);
    printf("\n\n\n\n\t\t\t\t   CREER MOT DE PASSE ADMIN\n\n");
    Color(7,0);
    printf("\tEntrez le nouveau mot de passe admin : ");
    char pass[STR_MAX];
    read_line(pass, sizeof(pass));
    FILE *f = fopen(ADMIN_PASS_FILE, "w");
    if (!f) return false;
    fprintf(f, "%s", pass);
    fclose(f);
    return true;
}

bool check_admin_password() {
    system("cls");
    Color(15,4);
    printf("\n\n\n\n\n\n\n\n\n\t\t\t\t                   AUTHENTIFICATION ADMIN                     \n\n");
    Color(7,0);
    if (!admin_password_exists()) {
        printf("\nAucun mot de passe admin present. Vous allez en creer un.\n\n");
        create_admin_password();
        printf("\nMot de passe enregistre. ...");
        getch();
        return true;
    }
    // read stored
    FILE *f = fopen(ADMIN_PASS_FILE, "r");
    if (!f) return false;
    char stored[STR_MAX]; stored[0] = '\0';
    if (fgets(stored, sizeof(stored), f) == NULL) { fclose(f); return false; }
    trim_newline(stored);
    fclose(f);

    // ask input (visible input to avoid masking bugs)
    printf("\n\t\t\t\t  Entrez le mot de passe admin : ");
    char input[STR_MAX]; read_line(input, sizeof(input));
    if (strcmp(input, stored) == 0) {
    	Color(10,0);
        printf("\n\n\t\t\t\t\tMot de passe est correcte, Bienvenue Admin !");
         Color(0,7);
         
	  
	  printf("\n\n\n\t\t\t\t\t\t\t  Appuyez sur une touche pour continuez >> \n");
Color(0,0);
        getch();
        return true;
    } else {
    	Color(12,0);
        printf("\n\n\t\t\t\t\t\tMot de passe incorrect !");
        Color(0,7);
         printf("\n\n\n\t\t\t<< Appuyez sur une touche pour retournez  \n");
        getch();
        return false;
    }
}

// -------------------- Helper: Reflow student pans when book removed --------------------
void adjust_student_pans_after_book_delete(struct etudiant *students, int scount, int deleted_ref) {
    for (int i=0;i<scount;i++) {
        if (students[i].pan == deleted_ref) {
            students[i].pan = 0; // their book was deleted -> empty panier
        } else if (students[i].pan > deleted_ref) {
            students[i].pan--; // shift reference down
        }
    }
}

// -------------------- Display Functions --------------------
void admin_display_books(struct library *books, int bcount) {
    system("cls");
    Color(7,0);
    printf("\n\n\t--- Liste des Livres (%d) ---\n\n", bcount);
    if (bcount==0) printf("\t(Aucun livre)\n");
    for (int i=0;i<bcount;i++) {
        printf("\t[%02d] %s  | %s  | pages: %d\n", books[i].ref, books[i].bookName, books[i].author, books[i].pages);
    }
    printf("\n\nAppuyez sur Entrée pour revenir...");
    while (getch()!=13) {}
}

void admin_display_students(struct etudiant *students, int scount, struct library *books, int bcount) {
    system("cls");
    Color(7,0);
    printf("\n\n\t--- Liste des Etudiants (%d) ---\n\n", scount);
    if (scount==0) printf("\t(Aucun etudiant)\n");
    for (int i=0;i<scount;i++) {
        printf("\t[ID:%02d] %s  | pass: %s  | panier: ", students[i].id, students[i].Name, students[i].pass);
        if (students[i].pan > 0 && students[i].pan <= bcount) {
            printf("%s (ref:%d)\n", books[students[i].pan-1].bookName, students[i].pan);
        } else {
            printf("(vide)\n");
        }
    }
    printf("\n\nAppuyez sur Entrée pour revenir...");
    while (getch()!=13) {}
}

// -------------------- Program Entry --------------------
int main(void) {
    // set console to default colors
    Color(7,0);

    struct library books[MAX_BOOKS];
    struct etudiant students[MAX_STUDENTS];
    int book_count = 0, student_count = 0;

    // initialize
    for (int i=0;i<MAX_BOOKS;i++) {
        books[i].bookName[0]='\0'; books[i].author[0]='\0';
        books[i].d=false; books[i].ref=0; books[i].pages=0;
    }
    for (int i=0;i<MAX_STUDENTS;i++) {
        students[i].Name[0]='\0'; students[i].pass[0]='\0';
        students[i].pan=0; students[i].id=0;
    }

    // load persisted data if present (text files)
    book_count = load_books_txt(books, MAX_BOOKS);
    student_count = load_students_txt(students, MAX_STUDENTS);
    // ensure refs and ids consistent if loaded
    for (int i=0;i<book_count;i++) books[i].ref = i+1;
    for (int i=0;i<student_count;i++) students[i].id = i+1;

    int main_pos = 1; // starting highlighted option
    while (1) {
        // main menu loop with arrow selection
        int selection = main_pos;
        int key = 0;
        while (key != 13) { // Enter
            Color(7,0); system("cls");
            // show counts at top
            Color(4,0);
            printf("\n\t Livres = %d ", book_count);
            printf("\n\n\t Etudiants = %d ", student_count);
            Color(7,4);
            printf("\n\n\n\n\n\n\n\n\n\n\t\t\t\t\t              Menu Principal                \n");
            Color(11,0);
            arrowHere(1, selection); printf("\n\t\t\t\t\t\t      Espace admin       \n");
            arrowHere(2, selection); printf("\n\t\t\t\t\t\t     Espace etudiant     \n");
            arrowHere(3, selection); printf("\n\t\t\t\t\t\t       Sauvegarder     \n");
            arrowHere(4, selection); printf("\n\t\t\t\t\t\t         Quitter      \n");
            key = read_arrow();
            if (key == 80 && selection < 4) selection++;
            else if (key == 72 && selection > 1) selection--;
        }
        main_pos = selection; // preserve last pos

        if (selection == 4) { // Quit
            // Save before exit
            save_books_txt(books, book_count);
            save_students_txt(students, student_count);
            system("cls");
            Color(7,0);
            printf("\nMerci. Fin du programme.\n");
            return 0;
        } else if (selection == 3) { // Save
            save_books_txt(books, book_count);
            save_students_txt(students, student_count);
            system("cls");
            Color(7,0);
            printf("\nDonnees sauvegardees sur disque (books.txt / students.txt).\nAppuyez sur une touche...");
            getch();
            continue;
        } else if (selection == 1) { // Admin space
            // Authenticate admin
            if (!check_admin_password()) continue;

            // Admin space loop: stays until admin chooses "Retour"
            bool stayInAdmin = true;
            int admin_sel = 1;

            while (stayInAdmin) {
                int akey = 0;
                while (akey != 13) {
                    Color(7,0); system("cls");
                    Color(7,1);
                    printf("\n\n\n\n\n\n\n\t\t\t\t                       ESPACE ADMIN                          ");
                    Color(11,0);
                    arrowHere(1, admin_sel); printf("\n\n\n\t\t\t\t\t           Ajouter un nouveau livre         \n");
                    arrowHere(2, admin_sel); printf("\n\t\t\t\t\t          Modifier / Supprimer Livre        \n");
                    arrowHere(3, admin_sel); printf("\n\t\t\t\t\t          Ajouter Un Compte Etudiant        \n");
                    arrowHere(4, admin_sel); printf("\n\t\t\t\t\t         Modifier / Supprimer Etudiant      \n");
                    arrowHere(5, admin_sel); printf("\n\t\t\t\t\t               Afficher Livres              \n");
                    arrowHere(6, admin_sel); printf("\n\t\t\t\t\t              Afficher Etudiants            \n");
                    arrowHere(7, admin_sel); printf("\n\n\t\t\t\t\t  << Retour   \n");
                    akey = read_arrow();
                    if (akey == 80 && admin_sel < 7) admin_sel++;
                    else if (akey == 72 && admin_sel > 1) admin_sel--;
                }

                // Process admin selection (each action returns back to admin menu)
                switch (admin_sel) {
                case 1: { // Add book
                    Color(7,0); system("cls"); Color(9,0);
                    if (book_count >= MAX_BOOKS) {
                        Color(12,0); printf("\nLimite livres atteinte.\nPress any key...");
                        getch(); break;
                    }
                    
                    Color(9,0);
                    printf("\n\n\n\n\n\n\t\t\t\t\t\t  Reference du livre :  #%d", book_count+1);
                    Color(8,0);
                    printf("\n\n\n\t\t\t\t\tEntrer le nom de livre = ");
                    Color(7,0);
                    read_line(books[book_count].bookName, STR_MAX);
                    Color(8,0);
                    printf("\n\n\t\t\t\t\tEntrer le nom de l'auteur = ");
                    Color(7,0);
                    read_line(books[book_count].author, STR_MAX);
                    Color(8,0);
	               printf("\n\n\t\t\t\t\tEnter pages = ");
                    Color(7,0);
                    if (scanf("%d", &books[book_count].pages) != 1) books[book_count].pages = 0;
                    clear_input();
                    books[book_count].ref = book_count + 1;
                    books[book_count].d = true;
                    book_count++;
                    Color(10,0);
                    printf("\n\n\t\t\t\t\t\tLivre ajoute avec succes !");
                    Color(0,7);
         printf("\n\n\n\t\t\t  << Appuyez sur une touche pour retournez  \n");
                    getch();
                    break;
                }
                case 2: { // Modify/Delete book
                    Color(7,0); system("cls");
                    if (book_count == 0) { printf("\nAucun livre.\n"); getch(); break; }
                    Color(9,0);
                    printf("\n\n\n\t\t\t\t\t\tListe Livres \n\n");
                    for (int i=0;i<book_count;i++) {
                    
					
        Color(10,0);
        printf("\n\t\t[ %d ]", books[i].ref);
		 Color(5,0);          	
        printf("\tbook name = ");
        Color(7,0);
		printf("%s", books[i].bookName);
		Color(5,0);
        printf("\t author name = ");
        Color(7,0);
        printf("%s", books[i].author);
        Color(5,0);
        printf("\t  pages = ");
         Color(7,0);
        printf("%d", books[i].pages);
        Color(8,0);  

        printf("\n\t\t\t--------------------------------------------------------------");
					}
					Color(8,0);  
                    printf("\n\n\n\tEntrer reference du livre a modifier/supprimer : ");
                    int ref; if (scanf("%d",&ref)!=1) ref=-1; clear_input();
                    if (ref <= 0 || ref > book_count) { printf("Reference invalide.\n"); getch(); break; }
                    // Choose action with arrow
                    int action = 1; int ak = 0;
                    while (ak != 13) {
                        Color(7,0); system("cls");
                        printf("\nLivre selectionne: [%d] %s\n\n", books[ref-1].ref, books[ref-1].bookName);
                        arrowHere(1, action); printf("\nModifier\n");
                        arrowHere(2, action); printf("\nSupprimer\n");
                        arrowHere(3, action); printf("\nRetour\n");
                        ak = read_arrow();
                        if (ak == 80 && action < 3) action++;
                        else if (ak == 72 && action > 1) action--;
                    }
                    if (action == 1) {
                        printf("\nNouveau nom : "); read_line(books[ref-1].bookName, STR_MAX);
                        printf("Nouvel auteur : "); read_line(books[ref-1].author, STR_MAX);
                        printf("Pages : "); if (scanf("%d",&books[ref-1].pages)!=1) books[ref-1].pages=0; clear_input();
                        printf("\nLivre modifie.\n"); getch();
                    } else if (action == 2) {
                        for (int i=ref-1;i<book_count-1;i++) books[i]=books[i+1];
                        book_count--;
                        adjust_student_pans_after_book_delete(students, student_count, ref);
                        printf("\nLivre supprime.\n"); getch();
                    } else { /* return */ }
                    break;
                }
                case 3: { // Add Student
                    Color(7,0); system("cls");
                    if (student_count >= MAX_STUDENTS) { printf("\nLimite etudiants atteinte.\n"); getch(); break; }
                    printf("\nNom Etudiant : ");
                    read_line(students[student_count].Name, STR_MAX);
                    printf("Mot de passe : ");
                    read_line(students[student_count].pass, STR_MAX);
                    students[student_count].pan = 0;
                    students[student_count].id = student_count + 1;
                    student_count++;
                    printf("\nEtudiant ajoute.\n"); getch();
                    break;
                }
                case 4: { // Modify/Delete Student
                    Color(7,0); system("cls");
                    if (student_count == 0) { printf("\nAucun etudiant.\n"); getch(); break; }
                    printf("\nListe Etudiants :\n");
                    for (int i=0;i<student_count;i++) printf("[ID:%d] %s\n", students[i].id, students[i].Name);
                    printf("\nEntrer ID etudiant a modifier/supprimer : ");
                    int id; if (scanf("%d",&id)!=1) id=-1; clear_input();
                    if (id <= 0 || id > student_count) { printf("ID invalide.\n"); getch(); break; }
                    int action = 1; int ak=0;
                    while (ak!=13) {
                        Color(7,0); system("cls");
                        printf("\nEtudiant selectionne: [ID:%d] %s\n\n", students[id-1].id, students[id-1].Name);
                        arrowHere(1, action); printf("\nModifier\n");
                        arrowHere(2, action); printf("\nSupprimer\n");
                        arrowHere(3, action); printf("\nRetour\n");
                        ak = read_arrow();
                        if (ak==80 && action<3) action++;
                        else if (ak==72 && action>1) action--;
                    }
                    if (action==1) {
                        printf("\nNouveau nom : "); read_line(students[id-1].Name, STR_MAX);
                        printf("Nouveau mot de passe : "); read_line(students[id-1].pass, STR_MAX);
                        printf("\nEtudiant modifie.\n"); getch();
                    } else if (action==2) {
                        for (int i=id-1;i<student_count-1;i++) students[i]=students[i+1];
                        student_count--;
                        printf("\nEtudiant supprime.\n"); getch();
                    } else { /* return */ }
                    break;
                }
                case 5: { // Display Books
                    admin_display_books(books, book_count);
                    break;
                }
                case 6: { // Display Students
                    admin_display_students(students, student_count, books, book_count);
                    break;
                }
                case 7: { // Retour -> leave admin space and return to main menu
                    stayInAdmin = false;
                    break;
                }
                default:
                    break;
                } // end switch admin_sel
            } // end while stayInAdmin
        } // end admin selection

        else if (selection == 2) { // Student space (login -> profile)
            // login screen
            int trycount = 0;
            int logged_index = -1;
            while (trycount < 3 && logged_index == -1) {
                Color(7,0); system("cls");
                Color(15,4);
                printf("\n\n\n\n\t\t\t\t\t      CONNEXION ETUDIANT\n\n");
                Color(7,0);
                char username[STR_MAX]; char password[STR_MAX];
                printf("\nNom d'utilisateur : "); read_line(username, STR_MAX);
                printf("Mot de passe : "); read_line(password, STR_MAX);
                // search
                for (int i=0;i<student_count;i++) {
                    if (strcmp(username, students[i].Name)==0 && strcmp(password, students[i].pass)==0) { logged_index = i; break; }
                }
                if (logged_index==-1) {
                    printf("\nNom ou mot de passe incorrect. (tentative %d/3)\n", trycount+1);
                    trycount++;
                    getch();
                }
            }
            if (logged_index == -1) {
                printf("\nEchec connexion. Retour au menu principal.\n"); getch(); continue;
            }

            // Student space loop: stays until logout (Deconnexion)
            bool stayInStudent = true;
            int prof_sel = 1;

            while (stayInStudent) {
                int pkey = 0;
                while (pkey != 13) {
                    Color(7,0); system("cls");
                    Color(7,9);
                    printf("\n\n\n\n\n\n\n\t\t\t\t                  TON PROFIL : %s                    ", students[logged_index].Name);
                    arrowHere(1, prof_sel); printf("\n\n\n\t\t\t\t\t Chercher un livre par son nom \n");
                    arrowHere(2, prof_sel); printf("\n\t\t\t\t\t Chercher un livre par nom d'auteur \n");
                    arrowHere(3, prof_sel); printf("\n\t\t\t\t\t Ajouter un livre a mon panier (emprunter) \n");
                    arrowHere(4, prof_sel); printf("\n\t\t\t\t\t Rendre un livre \n");
                    arrowHere(5, prof_sel); printf("\n\n\t\t\t\t\t\t\t\t Votre panier >> \n");
                    arrowHere(6, prof_sel); printf("\n\n\t\t\t << Deconnexion \n");
                    pkey = read_arrow();
                    if (pkey == 80 && prof_sel < 6) prof_sel++;
                    else if (pkey == 72 && prof_sel > 1) prof_sel--;
                }

                // handle profile actions (all actions return to student menu)
                switch (prof_sel) {
                case 1: { // search by book name
                    system("cls"); Color(7,0);
                    printf("Entrer nom du livre (exact): ");
                    char q[STR_MAX]; read_line(q, STR_MAX);
                    bool found=false;
                    for (int i=0;i<book_count;i++) {
                        if (strcmp(q, books[i].bookName)==0) {
                            printf("\nResult: [%d] %s | %s | pages:%d\n", books[i].ref, books[i].bookName, books[i].author, books[i].pages);
                            found=true;
                        }
                    }
                    if (!found) printf("\nAucun resultat.\n");
                    printf("\nAppuyez sur une touche...");
                    getch();
                    break;
                }
                case 2: { // search by author
                    system("cls"); Color(7,0);
                    printf("Entrer nom d'auteur (exact): ");
                    char q[STR_MAX]; read_line(q, STR_MAX);
                    bool found=false;
                    for (int i=0;i<book_count;i++) {
                        if (strcmp(q, books[i].author)==0) {
                            printf("\nResult: [%d] %s | %s | pages:%d\n", books[i].ref, books[i].bookName, books[i].author, books[i].pages);
                            found=true;
                        }
                    }
                    if (!found) printf("\nAucun resultat.\n");
                    printf("\nAppuyez sur une touche...");
                    getch();
                    break;
                }
                case 3: { // borrow by reference (one book per student)
                    system("cls"); Color(7,0);
                    if (students[logged_index].pan > 0) {
                        printf("\nVous avez deja un livre dans votre panier (ref %d). Rendez-le d'abord.\n", students[logged_index].pan);
                        getch(); break;
                    }
                    if (book_count == 0) { printf("\nAucun livre disponible.\n"); getch(); break; }
                    printf("\nListe Livres:\n");
                    for (int i=0;i<book_count;i++) printf("[%d] %s | %s\n", books[i].ref, books[i].bookName, books[i].author);
                    printf("\nDonner la reference du livre choisi : ");
                    int r; if (scanf("%d",&r)!=1) r=-1; clear_input();
                    if (r <= 0 || r > book_count) { printf("\nReference invalide.\n"); getch(); break; }
                    students[logged_index].pan = r;
                    printf("\n%s a ete ajoute a ton panier.\n", books[r-1].bookName);
                    getch();
                    break;
                }
                case 4: { // return
                    system("cls"); Color(7,0);
                    if (students[logged_index].pan == 0) {
                        printf("\nTu n'as pas de livre dans ton panier.\n"); getch(); break;
                    }
                    int r = students[logged_index].pan;
                    printf("\n%s a ete rendu avec succes.\n", books[r-1].bookName);
                    students[logged_index].pan = 0;
                    getch();
                    break;
                }
                case 5: { // show panier
                    system("cls"); Color(7,0);
                    if (students[logged_index].pan == 0) {
                        printf("\nTon panier est vide.\n");
                    } else {
                        int r = students[logged_index].pan;
                        printf("\nNom du Livre = %s\nAuteur = %s\nPages = %d\nReference = %d\n", books[r-1].bookName, books[r-1].author, books[r-1].pages, books[r-1].ref);
                    }
                    getch();
                    break;
                }
                case 6: { // Deconnexion -> leave student space
                    stayInStudent = false;
                    break;	
                }
                default:
                    break;
                } // end switch prof_sel
            } // end while stayInStudent

        } // end student selection

        // After any top-level action, loop returns to main menu
    } // main loop

    return 0;
}

