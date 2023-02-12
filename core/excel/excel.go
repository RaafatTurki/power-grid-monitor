package excel

import (
	"fmt"
  "time"
	"power-grid-monitor/lib/log"

	"github.com/xuri/excelize/v2"
)

func CreateFile() (*excelize.File, string) {
	file := excelize.NewFile()
	defer func() {
		if err := file.Close(); err != nil {
			log.PrintErr(err)
		}
	}()

	main_sheet_name := "Main"

	// Create a new sheet.
	index, err := file.NewSheet(main_sheet_name)
	log.PrintErr(err)

	// Set active sheet of the workbook.
	file.SetActiveSheet(index)

	return file, main_sheet_name
}

func AddTitlesRow(file *excelize.File, data_sheet_name string, row_number int, titles []string) {
  alphabet := []string{ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" }

	for i, title := range titles {
    file.SetCellValue(data_sheet_name, fmt.Sprintf("%s%d", alphabet[i], row_number), title)
	}
}

func AddStationStateRow(file *excelize.File, data_sheet_name string, row_number int, id uint, state uint, voltage float32, current float32, temp float32, timestamp time.Time) {
	file.SetCellValue(data_sheet_name, fmt.Sprintf("A%d", row_number), id)
	file.SetCellValue(data_sheet_name, fmt.Sprintf("B%d", row_number), state)
	file.SetCellValue(data_sheet_name, fmt.Sprintf("C%d", row_number), voltage)
	file.SetCellValue(data_sheet_name, fmt.Sprintf("D%d", row_number), current)
	file.SetCellValue(data_sheet_name, fmt.Sprintf("E%d", row_number), temp)
  file.SetCellValue(data_sheet_name, fmt.Sprintf("F%d", row_number), timestamp.String())
  file.SetCellValue(data_sheet_name, fmt.Sprintf("G%d", row_number), timestamp.UnixMilli())
}
