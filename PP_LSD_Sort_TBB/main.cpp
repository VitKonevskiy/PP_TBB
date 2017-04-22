#include <tbb\task_scheduler_init.h>
#include <tbb\parallel_sort.h>
#include <tbb\tick_count.h>
#include <time.h>
using namespace tbb;


#define RAND_MAX_DOUBLE 1000


#define PRINT_ARRAY 0
#define PRINT_FOR_COMPARE 0
#define N 8000
#define N_THREADS 4
#define PRINT_BEFORE_COLLECT 0



// ���������� ��������� ��� ���� double �� i-�� �����
void CountingSort(double* arr_inp, double* arr_out, int size_arr, int byte_num) 
{
	// ���� ���������:
	// ������ ���� ����� ����� 256 ��������� ��������� (�������� ����� �� 0 �� 255)
	// ����� ���� ����� � � � � �������� ��������� s_A � s_B ��������������
	// ���� A ������ ����� B, ���� s_A > s_B
	unsigned char* mas = (unsigned char*)arr_inp;

	int counter[256];// �������� 256 ��������� ��������� ������ �����
	int offset;

	memset(counter, 0, sizeof(int) * 256); // �������� ������

										   // mas ���������� ������ � ����� �� 0 �� 255 (����� ������� ����� ��������� �����)
	for (int i = 0; i < size_arr; i++)
		counter[mas[8 * i + byte_num]]++; // counter ����������, ������� ����� ���� double �������� ������������ ������

										  // ������ ���� ����� ��������� ����� byte_num, ������� ������������ � �����-���� ��������� double 
	int j = 0;
	for (j; j < 256; j++)
		if (counter[j] != 0)
			break;

	offset = counter[j];// ������ offset ����������, ������� ������� ��������� � ������������ ������ (����� ����������, ������� ����� ������� arr_out ����� ��� �����,
						// ���������� ���� � ������� ��������� j)
	counter[j] = 0;// ��� ������������� �������� ���������, ���������� ���� � ������� ��������� j. ������ ����� �������� ����� ����� "���������� ����" � ����� �������� � ������ ������� arr_out
	j++;

	// ����� ������� �������� � ���������� �� � counter

	for (j; j < 256; j++)
	{
		int tmp = counter[j];
		counter[j] = offset;
		offset += tmp;
	}

	for (int i = 0; i < size_arr; i++)
	{
		arr_out[counter[mas[8 * i + byte_num]]] = arr_inp[i];// counter �������� ��� ����������� ���������� �� ����������� ������������ ���������
		counter[mas[8 * i + byte_num]]++;// ����������� �������� �� 1 ��� �������� (����� ��������� ��� �������� � ������ ������� arr_out)
	}
}


void LSDSortDouble(double* arr_inp, int size_arr)
{
	// �������� ��� �������, ������� ����� ��������� �� ����������� ������������� � ������������� ��������
	// ��� ������� �� ��� �� 8 ��� �������� �������������� ���������� ���������
	// ����� ����� ������� ��� ������� � �������� ���������

	double* arr_inp_tmp;
	double* arr_out_tmp;
	int size_arr_plus = 0,
		size_arr_minus = 0;

	arr_inp_tmp = new double[size_arr];
	arr_out_tmp = new double[size_arr];

	for (int i = 0; i < size_arr; i++)
		arr_inp_tmp[i] = arr_inp[i];

	// ��������� ������
	CountingSort(arr_inp_tmp, arr_out_tmp, size_arr, 0);
	CountingSort(arr_out_tmp, arr_inp_tmp, size_arr, 1);
	CountingSort(arr_inp_tmp, arr_out_tmp, size_arr, 2);
	CountingSort(arr_out_tmp, arr_inp_tmp, size_arr, 3);
	CountingSort(arr_inp_tmp, arr_out_tmp, size_arr, 4);
	CountingSort(arr_out_tmp, arr_inp_tmp, size_arr, 5);
	CountingSort(arr_inp_tmp, arr_out_tmp, size_arr, 6);
	CountingSort(arr_out_tmp, arr_inp_tmp, size_arr, 7);

	//���������� ���������
	for (int i = 0; i < size_arr; i++)
		arr_inp[i] = arr_inp_tmp[i];

}


class LSDParallel :public task
{
private:
	double *mas;
	double *tmp;
	int size;
	int portion;

	void Collect(int size1, int size2)
	{
		for (int i = 0; i < size1; i++)
		{
			tmp[i] = mas[i];
		}


		double *mas2 = mas + size1;

/*		double *mas2 = new double [size2];

		for (int i = 0; i < size2; i++)
		{
			mas2[i] = mas[size1+i];
			printf("copy mas2: %d %lf\n", size1 + i, mas2[i]);
		}
		*/
		int a = 0;
		int b = 0;
		int i = 0;
		while ((a != size1) && (b != size2))
		{
			if (tmp[a] <= mas2[b])
			{
				mas[i] = tmp[a];
				a++;
			}
			else
			{
				mas[i] = mas2[b];
				b++;
			}
			i++;
		}
		if (a == size1)
		{
			for (int j = b; j < size2; j++)
			{
				mas[size1 + j] = mas2[j];
			}
		}
		else
		{
			for (int j = a; j < size1; j++)
			{
				mas[size2 + j] = tmp[j];
			}
		}
	}

public:
	//����������� �������������
	LSDParallel(double *_mas, double *_tmp, int _size,int _portion)
	{
		mas = _mas;
		tmp = _tmp;
		size = _size;
		portion=_portion;
	}


	task* execute()
	{
		if (size <= portion)
		{
			LSDSortDouble(mas, size);
		}
		else
		{
			LSDParallel &thread1 = *new (allocate_child()) LSDParallel(mas, tmp, (size / 2), portion);
			LSDParallel &thread2 = *new (allocate_child()) LSDParallel((mas + (size / 2)), (tmp + (size / 2)),(size - (size / 2)), portion);

			set_ref_count(3);
			spawn(thread1);
			spawn_and_wait_for_all(thread2);
			if (PRINT_BEFORE_COLLECT)
			{
				for (int i = 0; i < size; i++)
				{
					printf("adr = %d ", &mas[i]);
					fflush(stdout);
					printf(" BEFORE COLLECT: %lf , %d \n", mas[i], i);
					fflush(stdout);
				}
			}
			Collect(size / 2, size - size / 2);
		}
		return NULL;
	}
};

void LSDParallelSortDouble(double *inp, int size, int nThreads)
{
	
	double *out = new double[size];
	int portion = size / nThreads;
	if (size % nThreads != 0)
	{
		portion++;
	}
	LSDParallel& thread = *new (task::allocate_root())LSDParallel(inp, out, size, portion);
	task::spawn_root_and_wait(thread);

	


/*	printf("\n rooot");
	printf("\n");
	for (int i = 0; i < size; i++)
	{
		printf("%lf \n", out[i]);
		fflush(stdout);
	}
	printf("\n");
*/

	delete[] out;
	
	
}



static int compare(const void * a, const void * b)
{
	if (*(double*)a > *(double*)b) return 1;
	else if (*(double*)a < *(double*)b) return -1;
	else return 0;
}



int main(int argc,char* argv[])
{



	srand(time(0));
	double *array = new double[N];
	double *TestArray = new double[N];
	double *Test2Array = new double[N];
	bool TrueResult = true;
	for (int i = 0; i < N; i++)
	{
		array[i] = (double)(rand()) / RAND_MAX * RAND_MAX_DOUBLE;
		TestArray[i] = array[i];
		Test2Array[i] = array[i];
		if (PRINT_ARRAY)
		{
			printf("array adr = %d ", &array[i]);
			printf("%lf %d \n", array[i],i);
		}
	}
	printf("\n\n");

	//����� �������

	
	
	tick_count TimeParallelStart, TimeParallelEnd, time1,time2;

	task_scheduler_init init(N_THREADS);
	TimeParallelStart = tick_count::now();
	LSDParallelSortDouble(array, N, N_THREADS);
	TimeParallelEnd = tick_count::now();
	init.terminate();
	
	time1 = tick_count::now();
	qsort(TestArray, N, sizeof(double), compare);
	time2 = tick_count::now();


	printf("Time parallel = %f seconds\n", (TimeParallelEnd - TimeParallelStart).seconds());
	printf("qsort time = %f seconds\n", (time2 - time1).seconds());

	bool result = true;
	if (PRINT_FOR_COMPARE)
	{
		for (int i = 0; i < N; i++)
		{
			printf("%d   %lf   TRUE: %lf  \n", i, array[i] , TestArray[i]);
			if (array[i] != TestArray[i])
				result = false;
		}
	}
	printf("LSDDoubleParallelSort ? std::qsort      %i\n\n", result);
	system("pause");
	return 0;
}
