#include <columns.h>
#include <assert.h>

void clVisitChildren(
    const clHandler *encoder,
    const clColumn *column,
    void *context)
{
    switch (column->kind)
    {
        case cl_COLUMN_INT8:
        case cl_COLUMN_INT16:
        case cl_COLUMN_INT32:
        case cl_COLUMN_INT64:
        case cl_COLUMN_INT128:
        case cl_COLUMN_INT256:
        case cl_COLUMN_UINT8:
        case cl_COLUMN_UINT16:
        case cl_COLUMN_UINT32:
        case cl_COLUMN_UINT64:
        case cl_COLUMN_UINT128:
        case cl_COLUMN_UINT256:
        case cl_COLUMN_FLOAT8:
        case cl_COLUMN_FLOAT16:
        case cl_COLUMN_FLOAT32:
        case cl_COLUMN_FLOAT64:
        case cl_COLUMN_FLOAT128:
        case cl_COLUMN_FLOAT256:
        case cl_COLUMN_BOOL:
            return encoder->visitNumber(
                encoder, 
                column, 
                context);
        case cl_COLUMN_OBJECT:
            return encoder->visitObject(
                encoder, 
                column, 
                context);
        case cl_COLUMN_UNION:
            return encoder->visitUnion(
                encoder, 
                column, 
                context);
        case cl_COLUMN_FIXED_ARRAY:
            return encoder->visitFixedArray(
                encoder, 
                column, 
                context);
        case cl_COLUMN_FLEXIBLE_ARRAY:
            return encoder->visitFlexibleArray(
                encoder, 
                column, 
                context);
        default:
            assert(false);
    }
}
