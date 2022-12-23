#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/pytypes.h>
#include <armadillo>
#include "gtpsa_module.h"

namespace py = pybind11;

#if 0
std::cerr << "Analysing row" << std::endl;
size_t row_start = 0, row_stop = 0, row_step = 0, row_slicelength = 0;
if (!row.compute(mat.n_rows, &row_start, &row_stop, &row_step, &row_slicelength)) {
	throw py::error_already_set();
}
std::cerr << "Analysing col" << std::endl;
size_t col_start = 0, col_stop = 0, col_step = 0, col_slicelength = 0;
if (!col.compute(mat.n_cols, &col_start, &col_stop, &col_step, &col_slicelength)) {
	throw py::error_already_set();
}
std::cerr << "Row access start " << row_start << " step " << row_step << " length " << row_slicelength << std::endl;
std::cerr << "Col access start " << col_start << " step " << col_step << " length " << col_slicelength << std::endl;
#endif

static py::buffer_info from_arma_mat(arma::mat &mat)
{
	size_t n_cols = static_cast<size_t>(mat.n_cols);
	py::buffer_info r;
	r.ptr = mat.memptr();         /* Pointer to buffer */
	r.itemsize = sizeof(double); /* Size of one scalar */
	r.format = py::format_descriptor<double>::format(); /* Python struct-style format descriptor */
	r.ndim = 2;
	r.shape = {
            static_cast<py::ssize_t>(mat.n_rows),
		    static_cast<py::ssize_t>(mat.n_cols)
	};/* Number of dimensions */
	r.strides = {
		/* Strides (in bytes) for each index */
		static_cast<py::ssize_t>(sizeof(double)),
        // should that not be n_rows ?
		static_cast<py::ssize_t>(sizeof(double) * n_cols)};
	return r;
}

static py::buffer_info from_arma_cube(arma::cube &cube)
{
    size_t n_cols = static_cast<size_t>(cube.n_cols);
    size_t n_slices = static_cast<size_t>(cube.n_slices);
    py::buffer_info r;
    r.ptr = cube.memptr();         /* Pointer to buffer */
    r.itemsize = sizeof(double); /* Size of one scalar */
    r.format = py::format_descriptor<double>::format(); /* Python struct-style format descriptor */
    r.ndim = 3;
    r.shape = { static_cast<py::ssize_t>(cube.n_rows),
                static_cast<py::ssize_t>(cube.n_cols),
                static_cast<py::ssize_t>(cube.n_slices)
    };/* Number of dimensions */
    r.strides = {
            /* Strides (in bytes) for each index */
            static_cast<py::ssize_t>(sizeof(double)),
            static_cast<py::ssize_t>(sizeof(double) * n_cols),
            static_cast<py::ssize_t>(sizeof(double) * n_cols * n_slices),
            };
    return r;
}


static arma::mat mat_from_np_array(py::array_t<double, py::array::c_style|py::array::forcecast> buffer)
{
	/* Request a buffer descriptor from Python */
	py::buffer_info info = buffer.request();

	/* Some sanity checks ... */
	if (info.format != py::format_descriptor<double>::format())
		throw std::runtime_error("Incompatible format: expected a double array!");

	if (info.ndim != 2){
		std::stringstream strm;
		strm << "Incompatible buffer: expected 2 but got "
		     << info.ndim << "dimensions!";
		throw std::runtime_error(strm.str());
	}

	bool need_transpose = false;
	if(info.strides[0] != sizeof(double)){
		need_transpose = true;
	}
	/*
	  std::cerr << "Strides [" << info.strides[0] << ", " << info.strides[1] << "]"
	  << ": need_transpose " << std::boolalpha << need_transpose << std::endl;
	*/
	auto mat = arma::mat(static_cast<const double *>(info.ptr),
			     info.shape[0], info.shape[1]);
	if(need_transpose){
		arma::inplace_trans(mat);
	}
	return mat;
}


void py_gtpsa_init_arma(py::module &m)
{
	//arma::mat;

	py::class_<arma::mat>(m, "Matrix", py::buffer_protocol())
		.def("__repr__", [](arma::mat &mat) -> std::string {
					 std::stringstream strm;
					 mat.print(strm, "<tpsa array>");
					 return strm.str();
				 })
		.def("__copy__", [](arma::mat &mat) -> arma::dmat {
					 return arma::dmat(mat);
				 })
		.def("__getitem__", [](arma::mat &mat, py::sequence &seq) -> double {
					    py::int_ row = seq[0], col =  seq[1];
					    return mat(row, col);
				    })
		.def("__setitem__", [](arma::mat &mat, py::sequence &seq, const double val){
					    py::int_ row = seq[0], col =  seq[1];
					    mat(row, col) = val;
				    })
		.def("swap_rows", &arma::mat::swap_rows, "swap row1, row2")
		.def("swap_cols", &arma::mat::swap_cols, "swap col1, col2")
		//.def("__str__", &PyArma::pstr)
		.def_buffer(from_arma_mat )
		//.def(py::init([](py::buffer b) {
		.def(py::init([](py::array_t<double, py::array::c_style|py::array::forcecast> b) {
				      return mat_from_np_array(b);
			      }));

    py::class_<arma::cube>(m, "Cube", py::buffer_protocol())
            .def("__repr__", [](arma::cube &cube) -> std::string {
                std::stringstream strm;
                cube.print(strm, "<tpsa hessian>");
                return strm.str();
            })
            .def("__copy__", [](arma::cube &cube) -> arma::dmat {
                return arma::cube(cube);
            })
            .def("__getitem__", [](arma::cube &cube, py::sequence &seq) -> double {
                py::int_ row = seq[0], col =  seq[1], slice = seq[2];
                return cube(row, col, slice);
            })
            .def("__setitem__", [](arma::cube &cube, py::sequence &seq, const double val){
                py::int_ row = seq[0], col =  seq[1], slice = seq[2];
                cube(row, col, slice) = val;
            })
            //.def("swap_rows", &arma::mat::swap_rows, "swap row1, row2")
            //.def("swap_cols", &arma::mat::swap_cols, "swap col1, col2")
                    //.def("__str__", &PyArma::pstr)
            .def_buffer(from_arma_cube )
                    //.def(py::init([](py::buffer b) {
                    /*
            .def(py::init([](py::array_t<double, py::array::c_style|py::array::forcecast> b) {
                return cube_from_np_array(b);
            }))
                     */
            ;

}
/*
 * Local Variables:
 * mode: c++
 * c-file-style: "python"
 * End:
 */
