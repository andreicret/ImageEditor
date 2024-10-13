#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VAL 256
#define IMPS 32767

typedef struct{
	short int ln, col, val, **mx, ph_type, max, x1, x2, y1, y2;
	unsigned long long hstgr[VAL];
	FILE *file;
	char filename[VAL];
} ld_ph;

void swap_int(short int *a, short int *b)
{
	short int aux = *a;
	*a = *b;
	*b = aux;
}

short int load_check(FILE *photo)
{
	//verific ca am o imagine incarcata
	if (!photo) {
		printf("No image loaded");
		return -1;
	}

	return 0;
}

short int from_char_to_int(char s[])
{
	/*transforma numerele din formatul char
	din fgets in short int */

	int sw = 0;
	char aux[VAL];

	if (!s)
		return IMPS;

	//sterg semnul minus
	if (s[0] == '-') {
		sw = 1;
		strcpy(aux, s + 1);
		strcpy(s, aux);
	}

	short int n = 0, string_size = strlen(s);
	for (int i = 0; i < string_size; i++) {
		if (!(s[i] >= '0' && s[i] <= '9'))
			return IMPS; //returneaza valoare imposibila daca nu e cifra
		n = n * 10 + (s[i] - '0');
	}

	//pentru numere negative
	if (sw)
		return -1 * n;

	return n;
}

double clamp(double x)
{
	if (x > VAL - 1)
		return VAL - 1;
	if (x < 0)
		return 0;

	return x;
}

void memory_fail(short int exit_code)
{
	//pentru programare defensiva
	fprintf(stderr, "Operation failed");
	exit(exit_code);
}

short int **alloc_mx(short int n, short int m)
{
	/* returneaza adresa unei noi matrici alocate dinamic;
	imi foloseste pentru stocarea imaginii, dar si pentru
	diferite matrici auxiliare*/

	short int **v;
	v = malloc(n * sizeof(short int *));

	if (!v)
		memory_fail(-1);

	for (int i = 0; i < n; i++) {
		v[i] = calloc(m, sizeof(short int));
		if (!v[i])
			memory_fail(-1);
	}

	return v;
}

void free_mx(int n, short int **v)
{  // eliberare spatiu matrice
	for (int i = 0; i < n; i++)
		free(v[i]);

	free(v);
}

void clear_photo(ld_ph *p)
{
	//elibereaza memoria fisierului
	free_mx(p->ln, p->mx);
	fclose((*p).file);
}

int select_all(ld_ph *p)
{
	//selecteaza intreaga imagine
	if (load_check((*p).file))
		return -1;  // Nicio imagine incarcata

	(*p).x1 = 0;
	(*p).x2 = (*p).ln;
	(*p).y1 = 0;
	(*p).y2 = (*p).col;
	return 0;
}

int select_pixels(char trash[], ld_ph *p)
{
	//selecteaza o portiune din imagine
	if (load_check((*p).file))
		return -1;  // Nicio imagine incarcata

	//folosesc desfacerea in cuvinte pentru a obtine coordonatele
	short int x1, x2, y1, y2;
	char *ptr;
	ptr = strtok(trash, "\n ");
	ptr = strtok(NULL, "\n ");
	y1 = from_char_to_int(ptr);

	ptr = strtok(NULL, "\n ");
	x1 = from_char_to_int(ptr);

	ptr = strtok(NULL, "\n ");
	y2 = from_char_to_int(ptr);

	ptr = strtok(NULL, "\n ");
	x2 = from_char_to_int(ptr);

	//verifica ca variabilele sa contina caractere numerice
	if (x1 == IMPS || x2 == IMPS || y1 == IMPS || y2 == IMPS) {
		printf("Invalid command");
		return -1;
	}

	// interschimb la nevoie
	if (x1 > x2)
		swap_int(&x1, &x2);
	if (y1 > y2)
		swap_int(&y1, &y2);

	// verificare ca sunt corecte

	if (x1 < 0 || y1 < 0) {
		printf("Invalid set of coordinates");
		return -1;
	} else if ((*p).ph_type % 3) {
		if (x2 > p->ln || y2 > p->col || x1 == x2 || y1 == y2) {
			printf("Invalid set of coordinates");
			return -1;
		}
	} else {
		if (x2 > (*p).ln || y2 > (*p).col / 3 || x1 == x2 || y1 == y2) {
			printf("Invalid set of coordinates");
			return -1;
		}
	}

	if ((*p).ph_type % 3 == 0) {
		(*p).y1 = 3 * y1;  // sar peste valorile pixelilor precedenti
		(*p).y2 = 3 * y2;
	} else {
		(*p).y1 = y1;
		(*p).y2 = y2;
	}

	(*p).x1 = x1;
	(*p).x2 = x2;

	return 0;
}

void print_message(short int a, short int b, short int c, short int d)
{	//daca afisam asta in main linia trecea de 80 de caractere :D
	printf("Selected %hd %hd %hd %hd", a, b, c, d);
}

void ascii_read(ld_ph p)
{
	//citirea matricii imaginii ascii
	int lines = p.ln, columns = p.col;

	for (int i = 0; i < lines; i++)
		for (int j = 0; j < columns; j++)
			fscanf(p.file, "%hd", &p.mx[i][j]);
}

void binary_read(ld_ph p, const fpos_t pos)
{
	//citirea binara a matricii
	unsigned char num;  // stocheaza valori intre 0 si 255
	p.file = freopen(p.filename, "rb", p.file);

	if (!p.file)
		memory_fail(-1);

	fsetpos(p.file, &pos);  // repozitionez pentru a citi direct matricea binar

	/*citesc mai intai un byte (unsgined char),
	apoi transform in short int (2 bytes)*/

	for (int i = 0; i < p.ln; i++)
		for (int j = 0; j < p.col; j++) {
			fread(&num, 1, 1, p.file);
			p.mx[i][j] = (short int)num;
		}
}

void skip_comment(ld_ph p)
{
	//sare peste eventualele comentarii din antetul imaginii
	char c, trash_line[VAL];

	//sar peste caractere albe
	while ((c = fgetc(p.file)) != EOF && (c == '\n' || c == ' '))
		continue;

	/*ignor linia comentata, in caz contrar repozitionez cursorul pentru
	citirea valorilor in mod recursiv*/

	if (c == '#') {
		fgets(trash_line, sizeof(trash_line), p.file);
		skip_comment(p);

	} else {
		fseek(p.file, -1, SEEK_CUR);
	}
}

void load(char filename[], ld_ph *p)
{
	//type [0] stocheaza litera 'P', iar type[1] nr. magic
	char type[2];
	ld_ph aux = *p;

	if ((*p).file)
		clear_photo(&aux);

	aux.file = fopen(filename, "rt");

	if (!aux.file) {
		printf("Failed to load %s", filename);
		(*p).file = NULL;
		return;
	}

	strcpy(aux.filename, filename);

	fscanf(aux.file, "%s", type);
	aux.ph_type = (int)(type[1] - '0'); //type[1] - nr.magic

	//citesc tinand cont de eventualele comentarii
	skip_comment(aux);
	fscanf(aux.file, "%hd", &aux.col);
	skip_comment(aux);
	fscanf(aux.file, "%hd", &aux.ln);
	skip_comment(aux);
	fscanf(aux.file, "%hd", &aux.max);
	skip_comment(aux);

	if (aux.ph_type == 3 || aux.ph_type == 6)
		aux.col = aux.col * 3;

	//aloc spatiu pentru a citi matricea de valori
	aux.mx = alloc_mx(aux.ln, aux.col);

	if (aux.ph_type == 2 || aux.ph_type == 3) { //format ascii
		ascii_read(aux);
	} else {
		/*salvez pozitia cursorului deoarece voi redeschde fisierul in format
		binar, iar cursorul va fi altfel iar la inceput*/

		fpos_t pos;
		fgetpos(aux.file, &pos);
		binary_read(aux, pos);
	}
	//selectarea implicita a intregii suprafete
	select_all(&aux);

	*p = aux;

	printf("Loaded %s", filename);
}

void crop(ld_ph *p)
{
	ld_ph aux = *p;
	if (load_check(aux.file))
		return;  // Nicio imagine incarcata

	short int new_line, new_column, **v;

	//dimensiunile noii imagini
	new_line = aux.x2 - aux.x1;
	new_column = aux.y2 - aux.y1;

	v = alloc_mx(new_line, new_column);
	if (!v)
		memory_fail(-1);

	//salvez in matrice auxiliara ce trebuie
	for (int i = aux.x1; i < aux.x2; i++)
		for (int j = aux.y1; j < aux.y2; j++)
			v[i - aux.x1][j - aux.y1] = aux.mx[i][j];

	free_mx(aux.ln, aux.mx);
	aux.ln = new_line;
	aux.col = new_column;
	aux.mx = alloc_mx(aux.ln, aux.col);

	//mut inapoi in variabila imaginii ce a ramas
	for (int i = 0; i < new_line; i++)
		for (int j = 0; j < new_column; j++)
			aux.mx[i][j] = v[i][j];

	select_all(&aux);

	*p = aux;

	free_mx(new_line, v);
	printf("Image cropped");
}

void frequency_array(ld_ph *p)
{
	/*genereaza un vector de frecventa cu valorile imaginii
	pentru a-l folosi la histogram si equalize*/

	memset((*p).hstgr, 0, VAL * sizeof(unsigned long long));

	for (int i = 0; i < (*p).ln; i++)
		for (int j = 0; j < (*p).col; j++)
			(*p).hstgr[(*p).mx[i][j]]++;
}

void histogram(int stars, int bins, ld_ph p)
{
	if (load_check(p.file))
		return;  // Nicio imagine incarcata

	if (p.ph_type % 3 == 0) {
		printf("Black and white image needed");
		return;
	}

	//frecventa tuturor valorilor
	frequency_array(&p);
	unsigned long long sum, star_n_o, max_value = 0, *arr;

	//in arr memorez $bin valori care urmeaza sa fie afisate in histograma
	arr = malloc(bins * sizeof(unsigned long long));
	if (!arr)
		memory_fail(-1);

	for (int i = 0; i < VAL; i += VAL / bins) {
		//suma corespunzatoare fiecarui bin
		sum = 0;
		for (int j = 0; j < VAL / bins; j++)
			sum = sum + p.hstgr[i + j];

		arr[i / (256 / bins)] = sum;
		// aflu valoarea maxima ca sa pot folosi formula de stelute
		if (sum > max_value)
			max_value = sum;
	}

	for (int i = 0; i < bins; i++) {
		//determin cate stelute tb. afisate
		star_n_o = (int)((double)(arr[i] / (double)max_value) * stars);
		printf("%lld    |   ", star_n_o);

		for (unsigned long long i = 0; i < star_n_o; i++)
			printf("*");
		printf("\n");
	}
	free(arr);
}

void equalize(ld_ph *p)
{
	if (load_check((*p).file))
		return;  // Nicio imagine incarcata

	if ((*p).ph_type % 3 == 0) {
		printf("Black and white image needed");
		return;
	}

	ld_ph aux = (*p);
	unsigned long long partial_sum[VAL], area;
	double argmnt;

	/*pentru eficienta voi folosi un vector de sume partiale
	folosesc si functia pt. vector de frecventa*/
	frequency_array(&aux);
	partial_sum[0] = aux.hstgr[0];

	for (int i = 1; i < VAL; i++)
		partial_sum[i] = partial_sum[i - 1] + aux.hstgr[i];

	//aplic formula scrisa in enunt
	area = aux.ln * aux.col;
	argmnt = 1.0 / area * (VAL - 1);

	for (int i = 0; i < VAL; i++)
		partial_sum[i] = round(clamp(partial_sum[i] * argmnt));

	//pentru fiecare pixel din imagine de valoare a, salvez f(a)
	for (int i = 0; i < aux.ln; i++)
		for (int j = 0; j < aux.col; j++)
			aux.mx[i][j] = partial_sum[aux.mx[i][j]];

	printf("Equalize done");

	*p = aux;
}

void rotate_whole(ld_ph *p)
{
	//rotirea unei imagini intregi cu -90 de grade
	short int **v, **aux, new_line, new_column;

	if (p->ph_type % 3) {
		//pt imagini greyscale
		new_line = p->col;
		new_column = p->ln;
		//noua imagine va fi redimensionata, inaltimea comuta cu latimea
		v = alloc_mx(new_line, new_column);

		for (int i = 0; i < p->ln; i++)
			for (int j = 0; j < p->col; j++)
				v[p->col - j - 1][i] = p->mx[i][j];

	} else {
		//pentru imagini RGB
		new_line = p->col / 3;
		new_column = p->ln * 3;
		v = alloc_mx(new_line, new_column);

		for (int i = 0; i < p->ln; i++)
			for (int j = 0; j < p->col / 3; j++) {
				v[p->col / 3 - j - 1][i * 3] = p->mx[i][j * 3];
				v[p->col / 3 - j - 1][i * 3 + 1] = p->mx[i][j * 3 + 1];
				v[p->col / 3 - j - 1][i * 3 + 2] = p->mx[i][j * 3 + 2];
			}
	}

	aux = p->mx;
	free_mx(p->ln, aux);

	p->ln = new_line;
	p->col = new_column;

	p->mx = alloc_mx(p->ln, p->col);
	//copiez matricea rotita
	for (int i = 0; i < p->ln; i++)
		for (int j = 0; j < p->col; j++)
			p->mx[i][j] = v[i][j];

	free_mx(p->ln, v);
	select_all(p);
}

void rotate(short int new_line, ld_ph *p)
{
	/*ROTIREA UNEI SELECTII - voi salva intr-o imagine auxiliara (aux)
	numai selectia imaginii pentru a aplica rotate_whole*/
	short int  new_column;
	ld_ph aux;

	aux.file = p->file;

	//stabilesc atributele lui aux in functie de tipul imaginii
	if (p->ph_type % 3) {
		aux.ph_type = 2;//greyscale
		new_column = new_line;
		aux.ln = new_line;
		aux.col = new_column;
		aux.mx = alloc_mx(new_line, new_column);

	} else {
		aux.ph_type = 3;//RGB

		aux.mx = alloc_mx(new_line, 3 * new_line);
		aux.ln = new_line;
		aux.col = new_line * 3;
	}
	//salvez in aux numai selectia
		for (int i = p->x1; i < p->x2; i++)
			for (int j = p->y1; j < p->y2; j++)
				aux.mx[i - p->x1][j - p->y1] = p->mx[i][j];

		select_all(&aux);
		//rotesc intreaga imagine aux
		rotate_whole(&aux);
		//copiez inapoi in selectia lui p valorile din aux
		for (int i = p->x1; i < p->x2; i++)
			for (int j = p->y1; j < p->y2; j++)
				p->mx[i][j] = aux.mx[i - p->x1][j - p->y1];
		free_mx(new_line, aux.mx);
}

void rotate_signal(char trash[], ld_ph *p)
{
	//functia principala de rotate
	if (load_check(p->file))
		return;  // No image loaded

	//iau unghiul
	char *ptr = strtok(trash, "\n ");
	ptr = strtok(NULL, "\n ");
	if (!ptr) {
		printf("Invalid command");
		return;
	}

	short int num = from_char_to_int(ptr), new_line, new_column, aux;

	new_line = p->x2 - p->x1;
	new_column = p->y2 - p->y1;

	//verific unghiul
	if (num % 90) {
		printf("Unsupported rotation angle");
		return;
	}
	aux = num;

	//determin daca rotatia se face pe selectie sau in intregime
	if (!(p->x1 == 0 && p->y1 == 0 && p->x2 == p->ln && p->y2 == p->col)) {
		if (p->ph_type % 3 && new_column != new_line) {
			printf("The selection must be square");
			return;
		} else if (p->ph_type % 3 == 0 && new_line != new_column / 3) {
			printf("The selection must be square");
			return;
		}
		//determin de cate ori rotesc
		if (num % 360) {
			num = (4 - (num / 90)) % 4;
			for (int i = 0; i < num; i++)
				rotate(new_line, p); //rotirea selectiei
		} else {
			//daca unghiul e +-360, nu mai tb. rotire
			printf("Rotated %hd", aux);
			return;
		}

	} else { //rotirea intregii imagini
		if (num % 360) {
			num = (4 - (num / 90)) % 4; //de cate ori rotesc
			for (int i = 0; i < num; i++)
				rotate_whole(p);
		} else {
			printf("Rotated %hd", aux);
			return;
		}
	}
	printf("Rotated %hd", aux);
}

double apply_kernel(double **ker, short int i, short int j, ld_ph p)
{
	//aplic nucleul de imagine selectat anterior

	//vectori de directie
	short int dx[] = {-1, -1, -1, 0, 0, 0, 1, 1, 1},
						dy[] = {-1, 0, 1, -1, 0, 1, -1, 0, 1}, new_i,
						new_j;
	double num = 0;

	//iau vecinii pentru a calcula
	for (int k = 0; k < 9; k++) {
		new_i = i + dx[k];
		new_j = j + 3 * dy[k];
		num += ((*ker)[k] * p.mx[new_i][new_j]);
	}
	return num;
}

void apply_signal(char trash[], ld_ph *p)
{
	if (load_check((*p).file))
		return;
	char *param;
	double *ker, num;
	//determin natura parametrului de apply
	param = strtok(trash, "\n ");
	param = strtok(NULL, "\n ");
	if (!param) {
		printf("Invalid command");
		return;
	}
	ker = malloc(9 * sizeof(double));
	if (!ker)
		memory_fail(-1);
	//asociez lui ker valorile specifice parametrului de apply
	if (p->ph_type % 3) {
		printf("Easy, Charlie Chaplin");
		return;
	} else if (strcmp(param, "EDGE") == 0) {
		for (int i = 0; i < 9; i++)
			ker[i] = -1;
		ker[4] = 8;
		printf("APPLY EDGE done");

	} else if (strcmp(param, "SHARPEN") == 0) {
		for (int i = 0; i < 9; i++)
			if (i % 2 == 0)
				ker[i] = 0;
			else
				ker[i] = -1;
		ker[4] = 5;
		printf("APPLY SHARPEN done");
	} else if (strcmp(param, "BLUR") == 0) {
		double sth = 1.0 / 9;
		for (int i = 0; i < 9; i++)
			ker[i] = sth;
		printf("APPLY BLUR done");

	} else if (strcmp(param, "GAUSSIAN_BLUR") == 0) {
		for (int i = 0; i < 9; i++)
			if (i % 2 == 0)
				ker[i] = 1.0 / 16;
			else
				ker[i] = 2.0 / 16;
		ker[4] = 4.0 / 16;
		printf("APPLY GAUSSIAN_BLUR done");
	} else {
		printf("APPLY parameter invalid");
		return;
	}
	short int new_line, new_column, **v;
	//dimensiunile selectiei
	new_line = p->x2 - p->x1;
	new_column = p->y2 - p->y1;
	/*pentru a calcula noile valori ale pixelilor, trebuie o matrice auxiliara
	ca sa iau vecinii*/
	v = alloc_mx(new_line, new_column);
	if (!v)
		memory_fail(-1);
	for (int i = p->x1; i < p->x2; i++)
		for (int j = p->y1; j < p->y2; j++)
			v[i - p->x1][j - p->y1] = p->mx[i][j];
	//aplic nucleul pentru toate valorile RGB si evit marginile
	for (int i = p->x1; i < p->x2; i++)
		for (int j = p->y1; j < p->y2; j += 3)
			if (!(i == 0 || j == 0 || i == p->ln - 1 || j == p->col - 3)) {
				num = apply_kernel(&ker, i, j, *p);
				v[i - p->x1][j - p->y1] = (short int)round(clamp(num));
				num = apply_kernel(&ker, i, j + 1, *p);
				v[i - p->x1][j + 1 - p->y1] = (short int)round(clamp(num));
				num = apply_kernel(&ker, i, j + 2, *p);
				v[i - p->x1][j + 2 - p->y1] = (short int)round(clamp(num));
			}
	//copiez valorile obtinute
	for (int i = p->x1; i < p->x2; i++)
		for (int j = p->y1; j < p->y2; j++)
			p->mx[i][j] = v[i - p->x1][j - p->y1];
	free(ker);
	free_mx(new_line, v);
}

void file_header(char new_filename[], FILE **out, ld_ph p)
{
	*out = fopen(new_filename, "wt");

	if (!(*out))
		memory_fail(-1);

	fprintf(*out, "P%hd\n", p.ph_type);
	if (p.ph_type % 3 == 0)
		fprintf(*out, "%hd ", p.col / 3);
	else
		fprintf(*out, "%hd ", p.col);

	fprintf(*out, "%hd\n%hd\n", p.ln, p.max);
}

void binary_save(char new_filename[], ld_ph p)
{
	if (load_check(p.file))
		return;  // No image loaded
	FILE *out;
	unsigned char num;
	if (p.ph_type < 4)
		p.ph_type += 3;  //transfmorma pt binar

	file_header(new_filename, &out, p);

	//scriere binara, cu unsigned char pentru a scrie exact 1 byte

	for (int i = 0; i < p.ln; i++) {
		for (int j = 0; j < p.col; j++) {
			num = (unsigned char)p.mx[i][j];
			fwrite(&num, sizeof(unsigned char), 1, out);
		}
	}
	fclose(out);
	printf("Saved %s", new_filename);
}

void ascii_save(char new_filename[], ld_ph p)
{
	if (load_check(p.file))
		return;  // Nicio imagine incarcata

	FILE *out;

	if (p.ph_type > 4)
		p.ph_type -= 3;  // converteste in P2/P3
	file_header(new_filename, &out, p);

	for (int i = 0; i < p.ln; i++) {
		for (int j = 0; j < p.col; j++)
			fprintf(out, "%hd ", p.mx[i][j]);
		fprintf(out, "\n");
	}

	fclose(out);
	printf("Saved %s", new_filename);
}

void save_signal(char filename[], ld_ph p)
{
	/*primul subprogram pentru comanda SAVE
	decide daca salvarea se face ascii sau binary*/
	char *ptr, trash[VAL];

	if (load_check(p.file))
		return;
	/*desfacerea in cuvinte pentru a lua noul filename si eventual
	parametrul ascii*/

	ptr = NULL;
	fgets(trash, VAL, stdin);
	ptr = strtok(trash, "\n ");
	strcpy(filename, ptr);
	ptr = strtok(NULL, "\n ");
	if (ptr) { //daca exista parametrul ascii
		ascii_save(filename, p);
	} else {
		binary_save(filename, p);
	}
}

void exit_editor(ld_ph *p)
{
	//iesirea eleganta a programului
	ld_ph aux = *p;

	int a = load_check((*p).file);
		/// Nicio imagine incarcata

	if (!a)
		clear_photo(&aux);
	exit(0);
}

int main(void)
{
	char op[VAL], filename[VAL], *ptr, trash[VAL];
	ld_ph p;
	p.file = NULL;
	int stars, bins;

	while (1) {
		//citesc linie cu linie si apoi prelucrez
		fgets(op, VAL, stdin);
		strcpy(trash, op);
		ptr = strtok(op, "\n ");
		if (strcmp(ptr, "LOAD") == 0) {
			ptr = strtok(NULL, "\n ");
			if (ptr)
				load(ptr, &p);
			else
				printf("Invalid command");

		} else if (strcmp(ptr, "SELECT") == 0) {
			ptr = strtok(NULL, "\n ");
			if (!ptr) {
				printf("Invalid command");
			} else if (strcmp(ptr, "ALL") == 0) {
				if (!select_all(&p))
					printf("Selected ALL");
			} else {
				if (select_pixels(trash, &p) == 0) {
					if (p.ph_type % 3)
						print_message(p.y1, p.x1, p.y2, p.x2);
					else
						print_message(p.y1 / 3, p.x1, p.y2 / 3, p.x2);
				}
			}
		} else if (strcmp(ptr, "CROP") == 0) {
			crop(&p);
		} else if (strcmp(ptr, "HISTOGRAM") == 0) {
			if (!load_check(p.file)) {
				ptr = strtok(NULL, "\n ");
				stars = from_char_to_int(ptr);
				ptr = strtok(NULL, "\n ");
				bins = from_char_to_int(ptr);
				ptr = strtok(NULL, "\n ");
				if (bins != IMPS && stars != IMPS && !ptr) //exista stars, bins
					histogram(stars, bins, p);
				else
					printf("Invalid command");
			}
		} else if (strcmp(ptr, "EXIT") == 0) {
			exit_editor(&p);
		} else if (strcmp(ptr, "SAVE") == 0) {
			ptr = strtok(NULL, "\n ");
			strcpy(filename, ptr);
			ptr = strtok(NULL, "\n ");
			if (ptr)//exista parametrul ascii
				ascii_save(filename, p);
			else
				binary_save(filename, p);
		} else if (strcmp(ptr, "ROTATE") == 0) {
			rotate_signal(trash, &p);
		} else if (strcmp(ptr, "EQUALIZE") == 0) {
			equalize(&p);
		} else if (strcmp(ptr, "APPLY") == 0) {
			apply_signal(trash, &p);
		} else {
			printf("Invalid command");
		}
		printf("\n");
	}

	return 0;
}
