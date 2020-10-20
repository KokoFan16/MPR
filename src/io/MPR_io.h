/*
 * MPR_io.h
 *
 *  Created on: Jul 7, 2020
 *      Author: kokofan
 */

#ifndef SRC_IO_MPR_IO_H_
#define SRC_IO_MPR_IO_H_

MPR_return_code MPR_write(MPR_file file, int svi, int evi);

MPR_return_code MPR_read(MPR_file file, int svi);


#endif /* SRC_IO_MPR_IO_H_ */
