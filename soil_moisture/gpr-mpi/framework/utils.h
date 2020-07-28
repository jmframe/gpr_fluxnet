#ifndef H_UTILS
#define H_UTILS
struct vector
{

   double *d;
   int dim;
   vector(int dim) : dim(dim) { d = new double[dim];}
   ~vector() {delete [] d;}

   vector& operator=(const vector& rhs)
   {
     for(int i=0; i<dim; ++i)
       this->d[i] = rhs.d[i];
     return *this;
   }
   
   vector& operator+=(vector& rhs)
   {
     for(int i=0; i<dim; ++i)
       this->d[i] += rhs.d[i];
     return *this;
   }
   
   vector& operator-=(vector& rhs)
   {
     for(int i=0; i<dim; ++i)
       this->d[i] -= rhs.d[i];
     return *this;
   }
   
   vector& operator*=(double a)
   {
     for(int i=0; i<dim; ++i)
       this->d[i] *= a;
     return *this;
   }
   
   vector& operator/=(double a)
   {
     for(int i=0; i<dim; ++i)
       this->d[i] /= a;
     return *this;
   }
   
   vector operator+(vector& rhs)
   {
     vector tmp(dim);
     for(int i=0; i<dim; ++i)
       tmp.d[i] = this->d[i] + rhs.d[i];
     return tmp;
   }
   
   vector operator-(vector& rhs)
   {
     vector tmp(dim);
     for(int i=0; i<dim; ++i)
       tmp.d[i] = this->d[i] - rhs.d[i];
     return tmp;
   }

    double scalar_product(vector& rhs)
    {
     double sum = 0.0;   
     for(int i=0; i<dim; ++i)
       sum += this->d[i] * rhs.d[i];
     return sum;
    }

    void print()
    {
      for(int i=0; i<dim; ++i)
        printf("%0.15e ", this->d[i]);
      printf("\n");
    }
};
#endif
